#
# Formulate ternary spinodal equation ;without invoking the custom materials class


lo='3e-9' # meter
xFe_avg=0.2
xCr_avg=0.6
Temperature=973.15
Gnormal='50e3' # J/mol
Bnormal='1e-23' # mol.m^2/(s.J)
Vm='1e-5' # m^3/mol

interval=20
num_steps=2
dt='0.01'
[Mesh]
  type = GeneratedMesh
  dim = 1
  nx = 32
  xmin = 0
  xmax = 32
[]

[Panmoose]
   elements='Fe Cr Ni'
   f_name='F'
   Temperature_in_Kelvin = '${Temperature}'
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
    seed=211
    variable=xCr
   [../]
   [./w1_initial]
     type = ConstantIC
     variable=wFe
     value=-0.0248
   [../]
   [./w2_initial]
     type = ConstantIC
     variable=wCr
     value=0.288
   [../]
[]

[BCs]
        [./Periodic]
                [./c_bcs]
                        auto_direction = 'x'
                [../]
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
   kappa         = 11.25e-9
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
  #l_tol = 1.0e-4
  nl_rel_tol = 1.0e-6
  nl_abs_tol = 1e-20
  start_time = 0.0
  num_steps = ${num_steps}

  [./TimeStepper]
    type = IterationAdaptiveDT
    dt = ${dt}
    cutback_factor = 0.5
    growth_factor = 1.2
    optimal_iterations = 3
  [../]
[]

[Postprocessors]
        [./iterations]
                type = NumNonlinearIterations
        [../]
[]

[Outputs]
  exodus = true
  interval = ${interval}
[]
