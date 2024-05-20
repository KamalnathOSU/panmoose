
#include "MultiSpinodalAction.h"
// MOOSE includes
#include "Conversion.h"
#include "FEProblem.h"
#include "Factory.h"
#include "MooseObjectAction.h"
#include "MooseMesh.h"
#include "AddVariableAction.h"

#include "AuxiliarySystem.h"

#include "libmesh/string_to_enum.h"
#include "libmesh/fe.h"

registerMooseAction("panmooseApp", MultiSpinodalAction, "setup_spinodal");

InputParameters
MultiSpinodalAction::validParams()
{
  InputParameters params = Action::validParams();
  params.addClassDescription(
      "Set up the variable(s) and the kernels needed for multicomponent spinodal decomposition. "
			"Note that this action supports only the REVERSE_SPLIT formulation of the Cahn-Hilliard (CH) equation. "
			"This action is created from ConservedAction. "
      );
  // Get MooseEnums for the possible order/family options for this variable
  MooseEnum families(AddVariableAction::getNonlinearVariableFamilies());
  MooseEnum orders(AddVariableAction::getNonlinearVariableOrders());
  params.addParam<Real>("scaling", 1.0, "Specifies a scaling factor to apply to this variable");
  params.addParam<bool>("implicit", true, "Whether kernels are implicit or not");
  params.addParam<bool>(
      "use_displaced_mesh", false, "Whether to use displaced mesh in the kernels");
  params.addParamNamesToGroup("scaling implicit use_displaced_mesh", "Advanced");
  params.addCoupledVar("args", "Vector of nonlinear variable arguments this kernel depends on");
  params.deprecateCoupledVar("args", "coupled_variables", "02/27/2024");
  params.addParam<std::vector<SubdomainName>>("block",
                                              "Block restriction for the variables and kernels");

	params.addParam<std::string>("prefix_mobility","mobility","The prefix of the mobility properties name.");
  params.addRequiredParam<MaterialPropertyName>(
      "f_name", "Base name of the free energy function F defined in a free energy material");
	params.addParam<std::string>("prefix_kappa","kappa","The prefix of the mobility properties name.");
	params.addRequiredParam<std::string>("elements","Elements to be defined. ");
	params.addParam<std::string>("prefix_x","x","Prefix of the non-linear composition variable.");
	params.addParam<std::string>("prefix_w","w","Prefix of the non-linear variational derivative variable.");
	params.addParam<bool>("add_Temperature",true,"Add temperature field (TK) as a auxiliary variable. ");
	params.addParam<bool>("compute_total_energy",true,"Calculate total free energy in 'Ft' auxiliary variable."
							" 'Ft' auxiliary variable is automatically added.");
	params.addParam<std::string>("Temperature_in_Kelvin","100","Value of the temperature field in Kelvin scale.");
  return params;
}

MultiSpinodalAction::MultiSpinodalAction(const InputParameters & params)
  : Action(params),
	_x_prefix(getParam<std::string>("prefix_x")),
	_w_prefix(getParam<std::string>("prefix_w")),
	_add_aux_TK(getParam<bool>("add_Temperature")),
	_compute_Ft(getParam<bool>("compute_total_energy")),
    _scaling(getParam<Real>("scaling"))
{
      _fe_type = FEType(Utility::string_to_enum<Order>("FIRST"),
                        Utility::string_to_enum<FEFamily>("LAGRANGE"));
}

void
MultiSpinodalAction::act()
{
	std::string elements_str=getParam<std::string>("elements");
	std::vector<std::string> m_components = split_string(elements_str,' ');
		auto ncomp=m_components.size();
  //
  // Add variable(s)
  //
  if (_current_task == "add_variable")
  {
    for(int ic=0; ic<ncomp-1; ic++) {
		auto ele  = m_components[ic];
    auto type = AddVariableAction::variableType(_fe_type);
    auto var_params = _factory.getValidParams(type);
		NonlinearVariableName _var_name = _x_prefix + ele;
		NonlinearVariableName _chempot_name = _w_prefix + ele;
    var_params.set<MooseEnum>("family") = Moose::stringify(_fe_type.family);
    var_params.set<MooseEnum>("order") = _fe_type.order.get_order();
    var_params.set<std::vector<Real>>("scaling") = {_scaling};
    var_params.applySpecificParameters(parameters(), {"block"});

    // Create conserved variable _var_name
    _problem->addVariable(type, _var_name, var_params);

    // Create chemical potential variable for split form
    _problem->addVariable(type, _chempot_name, var_params);
		}//end of ncomp loop
  }
	//
	// Add Aux variables
	//
	else if (_current_task == "add_aux_variable")
	{
		if(_add_aux_TK){
			InputParameters var_params = _factory.getValidParams("MooseVariable");
			NonlinearVariableName _var_name = "TK";
			//Set parameters
			var_params.set<MooseEnum>("family") = Moose::stringify(_fe_type.family);
			var_params.set<MooseEnum>("order") = _fe_type.order.get_order();
			var_params.set<std::vector<Real>>("scaling") = {_scaling};
			var_params.applySpecificParameters(parameters(), {"block"});
			//Create aux variable
			_problem->addAuxVariable("MooseVariable",_var_name, var_params);
		}//end of _add_aux_TK
		if(_compute_Ft){
			InputParameters var_params = _factory.getValidParams("MooseVariable");
			NonlinearVariableName _var_name = "Ft";
			//Set parameters
			var_params.set<MooseEnum>("family") = "MONOMIAL";
			var_params.set<MooseEnum>("order") = "CONSTANT";
			var_params.set<std::vector<Real>>("scaling") = {_scaling};
			var_params.applySpecificParameters(parameters(), {"block"});
			//Create aux variable
			_problem->addAuxVariable("MooseVariable",_var_name, var_params);
		}//end of _compute_Ft
		//Add dependent concentration variable
    {
			InputParameters var_params = _factory.getValidParams("MooseVariable");
			AuxVariableName var_name = _x_prefix + m_components[ncomp-1];
			//Set parameters
      var_params.set<MooseEnum>("family") = Moose::stringify(_fe_type.family);
      var_params.set<MooseEnum>("order") = _fe_type.order.get_order();
      var_params.set<std::vector<Real>>("scaling") = {_scaling};
      var_params.applySpecificParameters(parameters(), {"block"});
			//Create aux variable
			_problem->addAuxVariable("MooseVariable", var_name, var_params);
		}
	}//end of add_aux_variable
  //
	// Add Auxkernels
	//
	else if (_current_task == "add_aux_kernel")
	{
		//Add Temperature field calculation
		if(_add_aux_TK){
		auto type = "ParsedAux";auto var_name="TK";
		InputParameters var_params = _factory.getValidParams(type);
		var_params.set<ExecFlagEnum>("execute_on") = {EXEC_INITIAL, EXEC_TIMESTEP_END};
		var_params.set<AuxVariableName>("variable") = var_name;
		var_params.set<std::string>("expression") = getParam<std::string>("Temperature_in_Kelvin");
		var_params.set<bool>("use_xyzt") = true;
		_problem->addAuxKernel(type,"compute_" + std::string(var_name) ,var_params);
		}//end of temperature aux kernel

		if(_compute_Ft){
		auto type = "TotalFreeEnergy";auto var_name="Ft";
		InputParameters var_params = _factory.getValidParams(type);
		var_params.set<ExecFlagEnum>("execute_on") = {EXEC_INITIAL, EXEC_TIMESTEP_END};
		var_params.set<AuxVariableName>("variable") = var_name;
		var_params.set<MaterialPropertyName>("f_name") = getParam<MaterialPropertyName>("f_name");
		{
			std::vector<MaterialPropertyName> kappa_names;
			std::vector<VariableName> interfacial_vars;
		for(unsigned int ic=0; ic<ncomp-1; ic++){
			kappa_names.push_back( getParam<std::string>("prefix_kappa") + std::to_string(ic+1) + "_c" ) ;
			interfacial_vars.push_back( _x_prefix + m_components[ic] );
		}
		var_params.set<std::vector<MaterialPropertyName>>("kappa_names") = kappa_names;
		var_params.set<std::vector<VariableName>>("interfacial_vars") = interfacial_vars ;
		}
		_problem->addAuxKernel(type,"compute_" + std::string(var_name) ,var_params);
		}//end of _compute_Ft

		// Calculate dependent concentration variable
		{
		auto type = "ParsedAux"; 
		AuxVariableName var_name = _x_prefix + m_components[ncomp-1];
		//Set parameters
		InputParameters var_params = _factory.getValidParams(type);
		var_params.set<ExecFlagEnum>("execute_on") = {EXEC_INITIAL, EXEC_TIMESTEP_END};
		var_params.set<AuxVariableName>("variable") = var_name;
		var_params.set<bool>("use_xyzt") = false;
			//Create ParsedAux expression
			{
				std::string expression = "1.0";
				std::vector<VariableName> newVector;
				for(int ic=0; ic<ncomp-1; ic++){
								expression = expression + " - " + _x_prefix + m_components[ic];
								newVector.push_back( _x_prefix+m_components[ic] );
				}
		var_params.set<std::string>("expression") = expression;
		var_params.set<std::vector<VariableName>>("coupled_variables") = newVector;
			}
		_problem->addAuxKernel(type,"compute_"+std::string(var_name),var_params);
		}//end of dependent concentration variable

	}//end of add_aux_kernel
  //
  // Add Kernels
  //
  else if (_current_task == "add_kernel")
{
				
	// Add time derivative kernel
	{
		for(int ic=0; ic<ncomp-1; ic++){
		std::string kernel_type = "CoupledTimeDerivative";
		NonlinearVariableName var_name = _x_prefix + m_components[ic] ;
		NonlinearVariableName wvar_name = _w_prefix + m_components[ic] ;
		std::string kernel_name = "time" + std::to_string(ic+1);
		InputParameters params = _factory.getValidParams(kernel_type);
		params.set<NonlinearVariableName>("variable") = wvar_name;
		params.set<std::vector<VariableName>>("v") = {var_name};
		params.applyParameters(parameters());
	
		_problem->addKernel(kernel_type, kernel_name, params);
		}//end of ic-loop
	}
	
	// Add SplitCHWRes kernel
	{
		for(int ic=0; ic<ncomp-1; ic++){
			for(int jc=0; jc<ncomp-1; jc++){
		std::string kernel_type = "SplitCHWRes";
	
		std::string kernel_name = _w_prefix + std::to_string(ic+1)
											+ std::to_string(jc+1)
											+ "_" + "res";
		NonlinearVariableName var_name = _w_prefix + m_components[ic];
		InputParameters var_params = _factory.getValidParams(kernel_type);
		var_params.set<NonlinearVariableName>("variable") = var_name;
		if(ic<=jc)
			var_params.set<MaterialPropertyName>("mob_name") = getParam<std::string>("prefix_mobility")
																+std::to_string(ic+1)
																+std::to_string(jc+1);
		else
			var_params.set<MaterialPropertyName>("mob_name") = getParam<std::string>("prefix_mobility")
																+std::to_string(jc+1)
																+std::to_string(ic+1);													
		if(ic !=jc){
		var_params.set<std::vector<VariableName>>("w") = { _w_prefix + m_components[jc]};
		}//ic!=jc
		var_params.applyParameters(parameters());
		_problem->addKernel(kernel_type, kernel_name, var_params);
			}//end of jc-loop
		}//end of ic-loop
		
	}
	

	// Add SplitCHParsed kernel
	{
		for(int ic=0; ic<ncomp-1;ic++){
		std::string kernel_type = "SplitCHParsed";
		NonlinearVariableName var_name = _x_prefix + m_components[ic];
		NonlinearVariableName wvar_name = _w_prefix + m_components[ic] ;
		std::string kernel_name = var_name + "_" + kernel_type;
		InputParameters var_params = _factory.getValidParams(kernel_type);
		var_params.set<NonlinearVariableName>("variable") = var_name;
		var_params.set<std::vector<VariableName>>("w") = {wvar_name};
		var_params.set<MaterialPropertyName>("kappa_name") = getParam<std::string>("prefix_kappa") + std::to_string(ic+1) + "_c" ;
		//Create coupled_variable vector
			{
				std::vector<VariableName> newVector;
				for(int jc=0; jc<ncomp-1; jc++)
					if(jc != ic)
						newVector.push_back( _x_prefix+m_components[jc] );
		var_params.set<std::vector<VariableName>>("coupled_variables") = newVector;
			}		
		var_params.applyParameters(parameters());
		_problem->addKernel(kernel_type, kernel_name, var_params);
		}
	}
				
}//end of add_kernels
  else if (_current_task == "add_postprocessor")
{
	if(_compute_Ft){
		std::string type = "ElementIntegralVariablePostprocessor";
		VariableName var_name = "Ft";
		std::string pp_name = "total_free_energy";

		InputParameters var_params = _factory.getValidParams(type);
		var_params.set<std::vector<VariableName>>("variable") = {var_name};
		var_params.set<ExecFlagEnum>("execute_on") = {EXEC_INITIAL, EXEC_TIMESTEP_END};
		_problem->addUserObject(type, pp_name, var_params);
	}
}//end of add_postprocessor
}//end of act()
