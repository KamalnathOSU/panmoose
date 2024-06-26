#include "GmTernaryExtendedNegative.h"

registerMooseObject("panmooseApp", GmTernaryExtendedNegative);

InputParameters
GmTernaryExtendedNegative::validParams()
{
  InputParameters params = Material::validParams();
  params.addClassDescription(" Define free energy of a ternary system in a given phase");
  
  params.addParam<std::string>("phase_name","Fcc","The name of the phase whose free energy needs to be defined.");
  params.addParam<FileName>("database_name","FeCrNi.tdb","Name of the database file.");
  params.addParam<std::string>("database_type","tdb","Type of the database file.");
  params.addParam<std::string>("elements","Fe Cr Ni","Elements to be defined.");
  params.addParam<Real>("scale_Gnormal",1.0,"Factor (J/mol) to normalize the free energy."); 
  params.addParam<Real>("scale_Bnormal",1.0,"Factor (S.I units) to normalize the atomic mobility."); 
  params.addParam<Real>("scale_Vm",1.0,"Molar volume");
  params.addParam<Real>("scale_lo",1.0,"Normalization length (m) ; Grid size");
  params.addParam<Real>("kappa",11.25e-9,"Gradient energy coefficient (J/m)");
  params.addParam<MaterialPropertyName>("f_name", "F", "f property name : Free energy");
  params.addRequiredCoupledVar("X1", "Molar fraction field variable 1");
  params.addRequiredCoupledVar("X2", "Molar fraction field variable 2");
  params.addRequiredCoupledVar("TK", "Temperature field");
  params.addParam<PostprocessorName>("x1_avg",0.33,"The average of X1 concentration field.");
  params.addParam<PostprocessorName>("x2_avg",0.33,"The average of X2 concentration field.");
  params.addParam<PostprocessorName>("TK_avg",1000,"The average temperature of the field.");
  
  return params;
}

GmTernaryExtendedNegative::~GmTernaryExtendedNegative()
  {
	if(_m_pfm_sdk != NULL){
		cout<<"Deleting PFM_SDK pointer ..."<<endl;
		Delete_PFM_SDK( _m_pfm_sdk );
		_m_pfm_sdk = NULL;
	}
	//~DerivativeMaterialInterface<Material>();
  }

GmTernaryExtendedNegative::GmTernaryExtendedNegative(const InputParameters & parameters)
  : DerivativeMaterialInterface<Material>(parameters),
    _phase_name(getParam<std::string>("phase_name")),
	_database_name(getParam<FileName>("database_name")),
	_database_type(getParam<std::string>("database_type")),
	_elements_str(getParam<std::string>("elements")),
	_Gnormal(getParam<Real>("scale_Gnormal")),
	_Bnormal(getParam<Real>("scale_Bnormal")),
	_Vm(getParam<Real>("scale_Vm")),
	_lo(getParam<Real>("scale_lo")),
	_kappa(getParam<Real>("kappa")),
	_pps_x1_avg(getPostprocessorValue("x1_avg")),
	_pps_x2_avg(getPostprocessorValue("x2_avg")),
	_pps_TK_avg(getPostprocessorValue("TK_avg")),
  _X1(coupledValue("X1")),
  _X2(coupledValue("X2")),
  _TK(coupledValue("TK")),
  _X1_var(coupled("X1")),
  _X2_var(coupled("X2")),
  _TK_var(coupled("TK")),
  _X1_name(getVar("X1", 0)->name()),
  _X2_name(getVar("X2", 0)->name()),
  _TK_name(getVar("TK", 0)->name()),
  _G(declareProperty<Real>("f_name")),
  _dGdX1(declarePropertyDerivative<Real>("f_name", _X1_name)),
  _dGdX2(declarePropertyDerivative<Real>("f_name", _X2_name)),
  _d2GdX1X1(declarePropertyDerivative<Real>("f_name", _X1_name, _X1_name)),
  _d2GdX1X2(declarePropertyDerivative<Real>("f_name", _X1_name, _X2_name)),
  _d2GdX2X2(declarePropertyDerivative<Real>("f_name", _X2_name, _X2_name)),
  _Mob11(declareProperty<Real>("mobility11")),
  _Mob12(declareProperty<Real>("mobility12")),
  _Mob22(declareProperty<Real>("mobility22")),
  _kappa1_c(declareProperty<Real>("kappa1_c")),
  _kappa2_c(declareProperty<Real>("kappa2_c"))
{
	_m_pfm_sdk = NULL;
	_ncomp = 0;
	_initialize_mobility = false;
  using namespace std;
//Input for PanDataNet
PF_ARGS m_args;

//Output prop mode
int SDK_mode = 1 + 4; //Also calculate thermodynamic factor
Set_PFM_SDK_Mode(&m_args,SDK_mode);

// PDN config
map<string,double> m_pdn_config;
m_pdn_config["x"] = 0.01;
m_pdn_config["T"] = 5.0;
m_pdn_config["f"] = 0.01;
int count=0; m_args.num_sdk_config=3;
	for(auto &config : m_pdn_config){
		strcpy(m_args.sdk_config_key[count], config.first.c_str() );
		m_args.sdk_config_val[count] = config.second;
		++count;	
	}

// Condition wrapper
	auto *m_wrapper = &(m_args.condition_wrapper);
	map<string,string> databases;
	databases = { {_database_name,_database_type}  };
	m_wrapper->num_db = 0;
	for (auto & database : databases ) {
	string f_name = database.first;
	string f_type = database.second;
	if (!(f_type == "tdb" || f_type == "pdb")) {
	continue;
	}
	std::cout << endl;
	std::cout << ">> " << "[Database Path]" << f_name << endl;
	std::cout << ">> " << "[Database Type]" << f_type << endl;
	strcpy(m_wrapper->db_key[m_wrapper->num_db], f_name.c_str());
	strcpy(m_wrapper->db_val[m_wrapper->num_db], f_type.c_str());
	m_wrapper->num_db++;
	}//end of m_databases loop


// Components
	cout<<"Elements:"<<endl;
	vector<string> m_components = split_string(_elements_str,' ');
	vector<double> avg_comp = { 0.33, 0.33, 0.34 };
	m_wrapper-> num_comp = m_components.size();
	_ncomp = m_components.size();
	for(int ic=0; ic < m_wrapper-> num_comp; ic++)
	{
		strcpy( m_wrapper->comp_names[ic], m_components[ic].c_str() );
		cout<<"\tComponent : "<<m_components[ic]<<endl;
	}
			

// Phase names
	vector<string> phases = {_phase_name};
	m_wrapper-> num_ph = phases.size();
	for(int ip=0; ip < phases.size(); ip++)
			strcpy( m_wrapper->ph_names[ip], phases[ip].c_str() );
	cout<<" Phase name :"<<_phase_name<<endl;

// Number of threads
	m_wrapper->num_thread = 1;

// Set therm_set_temp for more than one phase
	m_wrapper->therm_set_temp[0] = 1000;
	m_wrapper->therm_set_temp[1] = 900;
	m_wrapper->num_therm_set = 2;

    // Set initial condition
	for(int ic=0; ic< m_wrapper-> num_comp; ic++){
		strcpy( m_wrapper->initial_condition_key[ic], m_components[ic].c_str() );
		 m_wrapper->initial_condition_val[ic] = avg_comp[ic] ;
	}
	strcpy( m_wrapper->initial_condition_key[ m_wrapper->num_comp ], "T" );
	m_wrapper->initial_condition_val[ m_wrapper->num_comp ]  = m_wrapper->therm_set_temp[0];

    // Analytical expression
	m_wrapper->ana_expr_only = false;

	char msg_char[256]="";
	bool calculate_parallel_tangent=false;
	cout<<"Creating PFM_SDK pointer ..."<<endl;
	_m_pfm_sdk = Define_PFM_SDK( &m_args, calculate_parallel_tangent, msg_char);
	if(_m_pfm_sdk == NULL){
	mooseError("PFM_SDK:Error: Could not create PFM_SDK pointer.",msg_char);
	}
	
	// Implement composition order from input file
	auto comp_name_protocol = _m_pfm_sdk->get_comp_name_protocol(msg_char);
	for(int i=0; i<comp_name_protocol.num_keys;i++)
		strcpy(comp_name_protocol.key[i], m_components[i].c_str());
	_m_pfm_sdk->update_comp_name_protocal(&comp_name_protocol,msg_char);
	if( !string(msg_char).empty() ){
	string msg(msg_char);
	mooseError("Error:PFM_SDK:comp_name_protocol: ",msg);
	}

	vector<double> conc_vector = { _pps_x1_avg, _pps_x2_avg, 1.0-_pps_x1_avg-_pps_x2_avg};
	double TK = _pps_TK_avg; 	
    PFM_SDK_Output_Data output;
    calculate_local_eq(conc_vector,TK,output);
	_Mob1 = output.phase_mobility[0][0];
	_Mob2 = output.phase_mobility[0][1];
	_Mob3 = output.phase_mobility[0][2];
	cout<<"Calculated global point: (x1,x2,TK) :"<<"("<<_pps_x1_avg
												<<","<<_pps_x2_avg
												<<","<<TK<<")"<<endl;
	cout<<" Mobility M1 "<<_Mob1<<endl;
	cout<<" Mobility M2 "<<_Mob2<<endl;
	cout<<" Mobility M3 "<<_Mob3<<endl;
}//end of constructor

void
GmTernaryExtendedNegative::computeQpProperties()
{

  Real R = 8.31442;// Gas constant

  Real X1 = _X1[_qp];
  Real X2 = _X2[_qp];
  Real X3 = 1-X1-X2;
  vector<double> conc_vector = {X1,X2,X3};
  Real T  = _TK[_qp];
  
  //Declare memory for PFM_SDK output data
  PFM_SDK_Output_Data output;
  calculate_local_eq(conc_vector,T,output);

//Free energy
  _G[_qp] = output.phase_Gibbs_energy[0] / _Gnormal ;

//First derivative
  double mu_ref=output.mu[ _ncomp - 1];
  _dGdX1[_qp] = (output.mu[0] - mu_ref) / _Gnormal ;
  _dGdX2[_qp] = (output.mu[1] - mu_ref) / _Gnormal ;

//Second derivative

  {int engine_id=0;
  _d2GdX1X1[_qp] = -1.0*cal_d2G(output,engine_id,0,0) / _Gnormal;
  _d2GdX1X2[_qp] = -1.0*cal_d2G(output,engine_id,0,1) / _Gnormal;
  _d2GdX2X2[_qp] = -1.0*cal_d2G(output,engine_id,1,1) / _Gnormal;
  }

// Chemical mobility
	Real M1=_Mob1, M2=_Mob2, M3=_Mob3;
	
	_Mob11[_qp] = 1/_Vm*( X1*M1*(1-X1)*(1-X1) + X2*M2*(0-X1)*(0-X1) + (1.0-X1-X2)*M3*(0-X1)*(0-X1) );
	_Mob12[_qp] = 1/_Vm*( X1*M1*(1-X1)*(0-X2) + X2*M2*(0-X1)*(1-X2) + (1.0-X1-X2)*M3*(0-X1)*(0-X2) );
	_Mob22[_qp] = 1/_Vm*( X1*M1*(0-X2)*(0-X2) + X2*M2*(1-X2)*(1-X2) + (1.0-X1-X2)*M3*(0-X2)*(0-X2) );
	
	_Mob11[_qp] *= _Vm / _Bnormal;
	_Mob12[_qp] *= _Vm / _Bnormal;
	_Mob22[_qp] *= _Vm / _Bnormal;
//Gradient energy coefficient
	_kappa1_c[_qp] = _kappa * _Vm / ( _Gnormal * _lo * _lo);
	_kappa2_c[_qp] = _kappa * _Vm / ( _Gnormal * _lo * _lo); 

}
