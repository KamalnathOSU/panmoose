#include "panmooseApp.h"
#include "Moose.h"
#include "AppFactory.h"
#include "ModulesApp.h"
#include "MooseSyntax.h"

InputParameters
panmooseApp::validParams()
{
  InputParameters params = MooseApp::validParams();

  return params;
}

panmooseApp::panmooseApp(InputParameters parameters) : MooseApp(parameters)
{
  panmooseApp::registerAll(_factory, _action_factory, _syntax);
}

panmooseApp::~panmooseApp() {}

void
panmooseApp::registerAll(Factory & f, ActionFactory & af, Syntax & syntax)
{
  ModulesApp::registerAllObjects<panmooseApp>(f, af, syntax);
  Registry::registerObjectsTo(f, {"panmooseApp"});
  Registry::registerActionsTo(af, {"panmooseApp"});

  /* register custom execute flags, action syntax, etc. here */
}

void
panmooseApp::registerApps()
{
  registerApp(panmooseApp);
}

/***************************************************************************************************
 *********************** Dynamic Library Entry Points - DO NOT MODIFY ******************************
 **************************************************************************************************/
extern "C" void
panmooseApp__registerAll(Factory & f, ActionFactory & af, Syntax & s)
{
  panmooseApp::registerAll(f, af, s);
}
extern "C" void
panmooseApp__registerApps()
{
  panmooseApp::registerApps();
}
