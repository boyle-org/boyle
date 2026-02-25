boyle_cxx_test(
  NAME
    bicycle_state_test
  SRCS
    "state_test.cpp"
  DEPS
    bicycle_state
)

boyle_cxx_test(
  NAME
    bicycle_kinematics_model_test
  SRCS
    "kinematics_model_test.cpp"
  DEPS
    bicycle_kinematics_model
)

boyle_cxx_test(
  NAME
    bicycle_dynamics_model_test
  SRCS
    "dynamics_model_test.cpp"
  DEPS
    bicycle_dynamics_model
)
