boyle_cxx_test(
  NAME
    bicycle_route_line_cubic_acc_planner_test
  SRCS
    "route_line_cubic_acc_planner_test.cpp"
  DEPS
    bicycle_route_line_cubic_acc_planner
)

boyle_cxx_test(
  NAME
    bicycle_route_line_quintic_acc_planner_test
  SRCS
    "route_line_quintic_acc_planner_test.cpp"
  DEPS
    bicycle_route_line_quintic_acc_planner
)

boyle_cxx_test(
  NAME
    bicycle_route_line_cubic_offset_planner_test
  SRCS
    "route_line_cubic_offset_planner_test.cpp"
  DEPS
    bicycle_route_line_cubic_offset_planner
)

boyle_cxx_test(
  NAME
    bicycle_route_line_quintic_offset_planner_test
  SRCS
    "route_line_quintic_offset_planner_test.cpp"
  DEPS
    bicycle_route_line_quintic_offset_planner
)

boyle_cxx_test(
  NAME
    bicycle_route_line_kinematics_planner_test
  SRCS
    "route_line_kinematics_planner_test.cpp"
  DEPS
    bicycle_route_line_kinematics_planner
)
