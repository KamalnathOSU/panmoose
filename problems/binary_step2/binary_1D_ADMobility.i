# Formulate binary spinodal


lo='3e-9' # meter
xFe_avg=0.5
Temperature=700
Gnormal='50e3' # J/mol
Bnormal='1e-25' # mol.m^2/(s.J)
Vm='1e-5' # m^3/mol

interval=50
num_steps=1500
dt='0.01'
[Mesh]
  type = GeneratedMesh
  dim = 1
  nx = 32
  xmin = 0
  xmax = 32
[]

[Variables]
  [./xFe] # Fe Mole fraction 
  [../]
  [./w1]
  [../]
  [./TK]
          initial_condition = ${Temperature}
  [../]
[]

[AuxVariables]
        [./Ft]
                order = CONSTANT
                family = MONOMIAL
        [../]
[]

[ICs]
 [./c1_initial]
  type = RandomIC
  min=${fparse xFe_avg-0.001}
  max=${fparse xFe_avg+0.001}
  seed=210
  variable=xFe
 [../]
   [./w1_initial]
     type = ConstantIC
     variable=w1
     value=-0.0905
   [../]
[]

[BCs]
        [./Periodic]
                [./c_bcs]
                        auto_direction = 'x'
                [../]
        [../]
[]

[Kernels]
  [./c1_res]
    type = SplitCHParsed
    variable = xFe
    f_name = 'F'
    kappa_name = 'kappa1_c'
    w = w1
  [../]

  [./w11_res]
    type = ADSplitCHWRes
    variable = w1
    mob_name = 'mobility11'
  [../]
  [./time1]
    type = CoupledTimeDerivative
    variable = w1
    v = xFe
  [../]

  [./TK_evo]
    type = TimeDerivative
    variable = TK
  [../]
[]

[AuxKernels]
        [./compute_Ft]
                type=TotalFreeEnergy
                variable = Ft
                f_name='F'
                kappa_names='kappa1_c '
                interfacial_vars='xFe '
        [../]
[]

[Materials]
   [./compute_free_energy]
   type=GmBinaryADMobility
   phase_name='Bcc'
   database_name='../FeCrNi.tdb'
   database_type='tdb'
   elements='Fe Cr'
   f_name='F'
   X1=xFe
   TK=TK
   outputs='exodus'
   x1_avg=${xFe_avg}
   TK_avg=${Temperature}
   scale_Gnormal = ${Gnormal}  # J/mol
   scale_Bnormal = ${Bnormal}  # mol.m^2/(J.s)
   scale_Vm      = ${Vm} # m^3/mol
   scale_lo      = ${lo} # m 
   kappa         = '11.25e-9'
   [../]
[]

[Preconditioning]
  # active = ' '
  [./SMP]
    type = SMP
    full = true
  [../]
[]

[Executioner]
  type = Transient
  scheme = bdf2
  automatic_scaling = true
  solve_type = 'NEWTON'
  petsc_options_iname = '-pc_type -ksp_gmres_restart -sub_ksp_type
                         -sub_pc_type -pc_asm_overlap'
  petsc_options_value = 'asm        31                 preonly
                         ilu             1'

  l_max_its = 30
  #l_tol = 1.0e-6
  nl_rel_tol = 1.0e-6
  nl_abs_tol = 1e-20
  start_time = 0.0
  num_steps = ${num_steps}

  [./TimeStepper]
    type = IterationAdaptiveDT
    dt = ${dt}
    cutback_factor = 0.5
    growth_factor = 1.2
    optimal_iterations = 5
  [../]
[]

[Postprocessors]
        [./iterations]
                type = NumNonlinearIterations
        [../]
        [./total_free_energy]
                type = ElementIntegralVariablePostprocessor
                variable = Ft
        [../]
[]

[Outputs]
  exodus = true
  interval = ${interval}
[]
