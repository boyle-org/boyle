boyle_cxx_test(
  NAME
    bicycle_feedforward_controller_test
  SRCS
    "feedforward_controller_test.cpp"
  DEPS
    bicycle_feedforward_controller
)

boyle_cxx_test(
  NAME
    bicycle_pid_controller_test
  SRCS
    "pid_controller_test.cpp"
  DEPS
    bicycle_pid_controller
)

boyle_cxx_test(
  NAME
    bicycle_model_predictive_controller_test
  SRCS
    "model_predictive_controller_test.cpp"
  DEPS
    bicycle_model_predictive_controller
)
