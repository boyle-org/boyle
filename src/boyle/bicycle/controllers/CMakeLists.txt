boyle_cxx_library(
  NAME
    bicycle_feedforward_controller
  HDRS
    "feedforward_controller.hpp"
)

boyle_cxx_library(
  NAME
    bicycle_pid_controller
  HDRS
    "pid_controller.hpp"
)

boyle_cxx_library(
  NAME
    bicycle_model_predictive_controller
  HDRS
    "model_predictive_controller.hpp"
)
