
#pragma once

#include "Material.h"
#include "DerivativeMaterialInterface.h"
#include <vector>

#include "pfm_sdk.h"
#include "PanPhaseFieldArguments.h"
class GmTernary : public DerivativeMaterialInterface<Material>
{
public:
  static InputParameters validParams();
  GmTernary(const InputParameters & parameters);
  
  vector<string> split_string(const string &s, char delim) {
    vector<string> elems;
    std::stringstream ss(s);
    std::string item;
    while (getline(ss, item, delim)) {
        elems.push_back(item);
    }  
    return elems;
  }

protected:
  virtual void computeQpProperties();
  
  const std::string & _phase_name;
  const FileName &    _database_name;
  const std::string   _database_type;
  const std::string   _elements_str;
  
  const VariableValue & _X1;
  const VariableValue & _X2;
  const VariableValue & _TK;
  unsigned int _X1_var;
  unsigned int _X2_var;
  unsigned int _TK_var;
  const VariableName _X1_name;
  const VariableName _X2_name;
  const VariableName _TK_name;
  
  MaterialProperty<Real> & _G;
  MaterialProperty<Real> & _dGdX1;
  MaterialProperty<Real> & _dGdX2;
  MaterialProperty<Real> & _d2GdX1X1;
  MaterialProperty<Real> & _d2GdX1X2;
  MaterialProperty<Real> & _d2GdX2X2;
  
};
