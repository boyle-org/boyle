boyle_cxx_library(
  NAME
    bicycle_state
  HDRS
    "state.hpp"
)

boyle_cxx_library(
  NAME
    bicycle_kinematics_model
  HDRS
    "kinematics_model.hpp"
  DEPS
    bicycle_state
)

boyle_cxx_library(
  NAME
    bicycle_dynamics_model
  HDRS
    "dynamics_model.hpp"
  DEPS
    bicycle_state
)
