includes("utils/xmake.lua")

target("common_fsm")
    set_kind("headeronly")
    add_headerfiles("fsm.hpp")
target_end()
