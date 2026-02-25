set_project("Boyle")
set_version("0.0.1", {soname = true})
set_description("The Fundamental Math Basis for Autonomous Driving Vehicles and Robotics.")
set_xmakever("3.0.7")

includes("xmake/toolchains/*.lua")

set_allowedplats("linux", "macosx", "windows")
set_allowedarchs("x86_64", "arm64")

set_defaultmode("release")
add_rules("mode.debug", "mode.release", "mode.releasedbg", "mode.minsizerel")

set_languages("c23", "cxx23")
set_encodings("utf-8")
set_warnings("more", "error")

set_policy("build.ccache", true)
set_policy("build.warning", true)

option("boyle_check_params")
    set_default(false)
    set_showmenu(true)
    set_description("Enable parameters checking")
option_end()

option("boyle_build_testing")
    set_default(true)
    set_showmenu(true)
    set_description("Enable testing")
option_end()

option("boyle_build_shared")
    set_default(false)
    set_showmenu(true)
    set_description("Build using shared libraries")
option_end()

option("boyle_enable_install")
    set_default(true)
    set_showmenu(true)
    set_description("Enable install")
option_end()

option("boyle_use_boost_unordered")
    set_default(true)
    set_showmenu(true)
    set_description("Enable Boost.unordered")
option_end()

option("boyle_use_blas_lapack")
    set_default("OpenBLAS")
    set_showmenu(true)
    set_values("OpenBLAS", "Netlib", "MKL", "OFF")
    set_description("BLAS/LAPACK library to use (OpenBLAS, Netlib, MKL, OFF)")
option_end()

if is_config("boyle_check_params", true) then
    add_defines("BOYLE_CHECK_PARAMS=1")
else
    add_defines("BOYLE_CHECK_PARAMS=0")
end

includes("xmake/rules.lua")
includes("deps/xmake.lua")

includes("src/xmake.lua")
if is_config("boyle_build_testing", true) then
    includes("tests/xmake.lua")
end

add_rules("plugin.compile_commands.autoupdate", {outputdir = "$(projectdir)", lsp = "clangd"})
