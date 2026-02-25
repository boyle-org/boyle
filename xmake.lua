set_project("Boyle")
set_version("0.0.1", {soname = true})
set_description("The Fundamental Math Basis for Autonomous Driving Vehicles and Robotics.")
set_xmakever("3.1.0")

includes("xmake/toolchains/*.lua")

if not get_config("toolchain") then
    if is_host("linux") then
        set_toolchains("linux-gcc-x64")
    elseif is_host("macosx") then
        set_toolchains("darwin-clang-arm64")
    elseif is_host("windows") then
        set_toolchains("windows-msvc-x64")
    end
end

set_allowedplats("linux", "macosx", "windows")
set_allowedarchs("x86_64", "x64", "arm64")

set_defaultmode("release")
add_rules("mode.debug", "mode.release", "mode.releasedbg", "mode.minsizerel")

set_languages("c23", "cxx23")
set_encodings("utf-8")
set_warnings("more", "error")

set_policy("build.ccache", true)
set_policy("build.warning", true)
set_policy("package.keep_source", true)

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
    set_default("openblas")
    set_showmenu(true)
    set_values("openblas", "netlib", "mkl", "n")
    set_description("BLAS/LAPACK library to use (openblas, netlib, mkl, n)")
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

add_rules("plugin.compile_commands.autoupdate", {outputdir = os.scriptdir(), lsp = "clangd"})
