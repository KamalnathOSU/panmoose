
#pragma once

#include "Material.h"
#include "DerivativeMaterialInterface.h"
#include <vector>
#include <cstring>
//#include "ExpressionBuilder.h"
//#include "DerivativeParsedMaterialHelper.h"

#include "PostprocessorInterface.h"

#include "pfm_sdk.h"
#include "PanPhaseFieldArguments.h"
class GmBinaryExtended : public DerivativeMaterialInterface<Material>
{
public:
  static InputParameters validParams();
  GmBinaryExtended(const InputParameters & parameters);
  ~GmBinaryExtended();

protected:
  virtual void computeQpProperties();
  
  const std::string & _phase_name;
  const FileName &    _database_name;
  const std::string &   _database_type;
  const std::string &  _elements_str;
  const Real & _Gnormal;
  const Real & _Bnormal;
  const Real & _Vm;
  const Real & _lo;
  const Real & _kappa;

  const PostprocessorValue & _pps_x1_avg;
  const PostprocessorValue & _pps_TK_avg;

  const VariableValue & _X1;
  const VariableValue & _TK;
  unsigned int _X1_var;
  unsigned int _TK_var;
  const VariableName _X1_name;
  const VariableName _TK_name;
  
  MaterialProperty<Real> & _G;
  MaterialProperty<Real> & _dGdX1;
  MaterialProperty<Real> & _d2GdX1X1;

  MaterialProperty<Real> & _Mob11;
  MaterialProperty<Real> & _kappa1_c;
	
private:
  Real  _Mob1;
  Real  _Mob2;
  bool _initialize_mobility;
	PFM_SDK* _m_pfm_sdk;  
  int _ncomp;
  vector<string> split_string(const string &s, char delim) {
    vector<string> elems;
    std::stringstream ss(s);
    std::string item;
    while (getline(ss, item, delim)) {
        elems.push_back(item);
    }  
    return elems;
  }
  double cal_d2G( PFM_SDK_Output_Data& output, int engine_id, int ic, int jc){
    double d2G=0.0;
    int cc = _ncomp-1;
    auto ThF=output.phase_thermodynamic_factor[engine_id];
    double ThF_ic_jc = ThF[ ic*_ncomp + jc];
    double ThF_cc_cc = ThF[ cc*_ncomp + cc];
    double ThF_ic_cc = ThF[ ic*_ncomp + cc];
    double ThF_cc_jc = ThF[ cc*_ncomp + jc];
    d2G = ThF_ic_jc - ThF_ic_cc - ThF_cc_jc + ThF_cc_cc;
    return d2G;
  }

void calculate_local_eq(vector<double> conc_vector, double TK, PFM_SDK_Output_Data &output) {

int ncomp=conc_vector.size();
PFM_SDK_Input_Condition input;
  int _thread_id=0;
  int _engine_id=0;
//Input
input.TK = TK;
for (int ic = 0; ic < ncomp ; ++ic) 
	input.composition[ic] = conc_vector[ic];
input.order_parameter[0] = 1;

//PFM_SDK
char msg_char[256] = ""; // message from sdk api
_m_pfm_sdk->get_pt_val(_thread_id, _engine_id,
				&input, &output,
				msg_char);
string msg(msg_char);
if( !msg.empty() ){
	mooseError("PFM_SDK: calculation failed. ");
}
  }//end of calculate_local_eq
};
