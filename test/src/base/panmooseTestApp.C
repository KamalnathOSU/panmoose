//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html
#include "panmooseTestApp.h"
#include "panmooseApp.h"
#include "Moose.h"
#include "AppFactory.h"
#include "MooseSyntax.h"
#include "ModulesApp.h"

InputParameters
panmooseTestApp::validParams()
{
  InputParameters params = panmooseApp::validParams();
  return params;
}

panmooseTestApp::panmooseTestApp(InputParameters parameters) : MooseApp(parameters)
{
  panmooseTestApp::registerAll(
      _factory, _action_factory, _syntax, getParam<bool>("allow_test_objects"));
}

panmooseTestApp::~panmooseTestApp() {}

void
panmooseTestApp::registerAll(Factory & f, ActionFactory & af, Syntax & s, bool use_test_objs)
{
  panmooseApp::registerAll(f, af, s);
  if (use_test_objs)
  {
    Registry::registerObjectsTo(f, {"panmooseTestApp"});
    Registry::registerActionsTo(af, {"panmooseTestApp"});
  }
}

void
panmooseTestApp::registerApps()
{
  registerApp(panmooseApp);
  registerApp(panmooseTestApp);
}

/***************************************************************************************************
 *********************** Dynamic Library Entry Points - DO NOT MODIFY ******************************
 **************************************************************************************************/
// External entry point for dynamic application loading
extern "C" void
panmooseTestApp__registerAll(Factory & f, ActionFactory & af, Syntax & s)
{
  panmooseTestApp::registerAll(f, af, s);
}
extern "C" void
panmooseTestApp__registerApps()
{
  panmooseTestApp::registerApps();
}
