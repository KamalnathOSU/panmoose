[Mesh]
  type = GeneratedMesh
  dim = 2
  nx = 5
  ny = 5
[]

[Variables]
  [u]
  []
  [X1]
          # x(Fe)
          initial_condition=0.2
  []
  [X2]
          # x(Cr)
          initial_condition=0.6
  []
  [TK]
          initial_condition=973.15
  []
[]

[Materials]
   [free_energy]
   type=GmTernaryExtended
   phase_name='Fcc'
   database_name='FeCrNi.tdb'
   database_type='tdb'
   elements='Fe Cr Ni'
   f_name='F'
   X1=X1
   X2=X2
   TK=TK
   outputs='exodus'
   x1_avg=x1_avg
   x2_avg=x2_avg
   TK_avg=TK_avg
   []
[]
[Kernels]
  [diff]
    type = Diffusion
    variable = u
  []
  [diff1]
    type = Diffusion
    variable = X1
  []
  [diff2]
    type = Diffusion
    variable = X2
  []
  [diff3]
    type = Diffusion
    variable = TK
  []
[]


[BCs]
  [left]
    type = DirichletBC
    variable = u
    boundary = left
    value = 0
  []
  [right]
    type = DirichletBC
    variable = u
    boundary = right
    value = 1
  []
[]

[Executioner]
  type = Steady
  solve_type = 'PJFNK'
  petsc_options_iname = '-pc_type -pc_hypre_type'
  petsc_options_value = 'hypre boomeramg'
[]

[Postprocessors]
        [./x1_avg]
                type = ElementAverageValue
                execute_on = 'INITIAL TIMESTEP_END'
                variable = X1
        [../]
        [./x2_avg]
                type = ElementAverageValue
                execute_on = 'INITIAL TIMESTEP_END'
                variable = X2
        [../]
        [./TK_avg]
                type = ElementAverageValue
                execute_on = 'INITIAL TIMESTEP_END'
                variable = TK
        [../]
[]

[Outputs]
  exodus = true
[]
