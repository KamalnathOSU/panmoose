#
# Formulate ternary spinodal equation ;without invoking the custom materials class


lo='3e-9' # meter
xFe_avg=0.2
xCr_avg=0.6
Temperature=973.15
Gnormal='50e3' # J/mol
Bnormal='1e-23' # mol.m^2/(s.J)
Vm='1e-5' # m^3/mol

interval=50
num_steps=1000
dt='10'
[Mesh]
  type = GeneratedMesh
  dim = 2
  nx = 32
  ny = 32
  xmin = 0
  xmax = 32
  ymin = 0
  ymax = 32
  elem_type = QUAD4
[]

[Variables]
  [./xFe] # Fe Mole fraction 
  [../]
  [./xCr] # Cr Mole fraction
  [../]
  [./w1]
  [../]
  [./w2]
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
  min=${fparse xFe_avg-0.0001}
  max=${fparse xFe_avg+0.0001}
  seed=210
  variable=xFe
 [../]
  [./c2_initial]
    type = RandomIC
    min=${fparse xCr_avg-0.0001}
    max=${fparse xCr_avg+0.0001}
    seed=210
    variable=xCr
   [../]
   [./w1_initial]
     type = ConstantIC
     variable=w1
     value=0.1284
   [../]
   [./w2_initial]
     type = ConstantIC
     variable=w2
     value=0.5962
   [../]
[]

[BCs]
        [./Periodic]
                [./c_bcs]
                        auto_direction = 'x y'
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
    coupled_variables = 'xCr'
  [../]
  [./w11_res]
    type = SplitCHWRes
    variable = w1
    mob_name = 'mobility11'
  [../]
  [./w12_res]
    type = SplitCHWRes
    variable = w1
    w = w2
    mob_name = 'mobility12'
  [../]

  [./c2_res]
    type = SplitCHParsed
    variable = xCr
    f_name = 'F'
    kappa_name = 'kappa2_c'
    w = w2
    coupled_variables = 'xFe'
  [../]
  [./w22_res]
    type = SplitCHWRes
    variable = w2
    mob_name = 'mobility22'
  [../]
  [./w21_res]
    type = SplitCHWRes
    variable = w2
    w = w1
    mob_name = 'mobility12'
  [../]

  [./time1]
    type = CoupledTimeDerivative
    variable = w1
    v = xFe
  [../]
  [./time2]
    type = CoupledTimeDerivative
    variable = w2
    v = xCr
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
                kappa_names='kappa1_c kappa2_c'
                interfacial_vars='xFe xCr'
        [../]
[]

[Materials]
   [./free_energy]
   type=GmTernaryExtended
   phase_name='Bcc'
   database_name='FeCrNi.tdb'
   database_type='tdb'
   elements='Fe Cr Ni'
   f_name='F'
   X1=xFe
   X2=xCr
   TK=TK
   outputs='exodus'
   x1_avg=${xFe_avg}
   x2_avg=${xCr_avg}
   TK_avg=${Temperature}
   scale_Gnormal = ${Gnormal}  # J/mol
   scale_Bnormal = ${Bnormal}  # mol.m^2/(J.s)
   scale_Vm      = ${Vm} # m^3/mol
   scale_lo      = ${lo} # m 
   kappa         = 11.25e-10
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
  l_tol = 1.0e-4
  nl_rel_tol = 1.0e-6
  nl_abs_tol = 1e-10
  start_time = 0.0
  num_steps = ${num_steps}

  dt = ${dt}
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
