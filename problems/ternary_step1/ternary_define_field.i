#
# Formulate ternary spinodal equation ;without invoking the custom materials class


lo='3e-9' # meter
xFe_avg=0.2
xCr_avg=0.6
Temperature=973.15
Gnormal='50e3' # J/mol
Bnormal='1e-24' # mol.m^2/(s.J)
Vm='1e-5' # m^3/mol
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
  [./c1] # Fe Mole fraction 
  [../]
  [./c2] # Cr Mole fraction
  [../]
  [./w1]
  [../]
  [./w2]
  [../]
  [./TK]
          initial_condition = ${Temperature}
  [../]
[]

[ICs]
 [./c1_initial]
  type = RandomIC
  min=${fparse xFe_avg-0.0001}
  max=${fparse xFe_avg+0.0001}
  seed=210
  variable=c1
 [../]
  [./c2_initial]
    type = RandomIC
    min=${fparse xCr_avg-0.0001}
    max=${fparse xCr_avg+0.0001}
    seed=210
    variable=c2
   [../]
[]

[Kernels]
  [./c1_res]
    type = SplitCHParsed
    variable = c1
    f_name = 'F'
    kappa_name = 'kappa1_c'
    w = w1
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
    variable = c2
    f_name = 'F'
    kappa_name = 'kappa2_c'
    w = w2
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
    v = c1
  [../]
  [./time2]
    type = CoupledTimeDerivative
    variable = w2
    v = c2
  [../]
  [./TK_evo]
    type = TimeDerivative
    variable = TK
  [../]
[]

[Materials]
   [./free_energy]
   type=GmTernaryExtended
   phase_name='Fcc'
   database_name='FeCrNi.tdb'
   database_type='tdb'
   elements='Fe Cr Ni'
   f_name='F'
   X1=c1
   X2=c2
   TK=TK
   outputs='exodus'
   x1_avg=${xFe_avg}
   x2_avg=${xCr_avg}
   TK_avg=${Temperature}
   scale_Gnormal = ${Gnormal}  # J/mol
   scale_Bnormal = ${Bnormal}  # mol.m^2/(J.s)
   scale_Vm      = ${Vm} # m^3/mol
   scale_lo      = ${lo} # m 
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

  solve_type = 'NEWTON'
  petsc_options_iname = -pc_type
  petsc_options_value = lu

  l_max_its = 30
  l_tol = 1.0e-4
  nl_rel_tol = 1.0e-10
  start_time = 0.0
  num_steps = 10

  dt = '1e-10'
[]

[Outputs]
  exodus = true
[]
