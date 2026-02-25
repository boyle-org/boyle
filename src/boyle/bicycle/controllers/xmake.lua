target("bicycle_feedforward_controller")
    set_kind("headeronly")
    add_headerfiles("feedforward_controller.hpp")
target_end()

target("bicycle_pid_controller")
    set_kind("headeronly")
    add_headerfiles("pid_controller.hpp")
target_end()

target("bicycle_model_predictive_controller")
    set_kind("headeronly")
    add_headerfiles("model_predictive_controller.hpp")
target_end()
