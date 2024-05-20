#pragma once

// MOOSE includes
#include "Action.h"
#include "libmesh/fe_type.h"

class MultiSpinodalAction : public Action
{
public:
  static InputParameters validParams();

  MultiSpinodalAction(const InputParameters & params);

  virtual void act() override;

protected:
	//Prefix for "x" mole fraction variable in split solve
  const std::string & _x_prefix;
	//Prefix of "w" variable in split solve
	const std::string &  _w_prefix;

	const bool & _add_aux_TK;
  const bool & _compute_Ft;
  /// FEType for the variable being created
  FEType _fe_type;
  /// Scaling parameter
  const Real _scaling;
private:

	std::vector<std::string> split_string(const std::string &s, char delim) {
					std::vector<std::string> elems;
    std::stringstream ss(s);
    std::string item;
    while (getline(ss, item, delim)) {
        elems.push_back(item);
    }  
    return elems;
  }
};
