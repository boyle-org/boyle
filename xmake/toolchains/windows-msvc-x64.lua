toolchain("windows-msvc-x64")
    set_kind("standalone")
    set_homepage("https://visualstudio.microsoft.com/")
    set_description("Windows MSVC x86_64")

    on_check(function (toolchain)
        return is_host("windows") and is_arch("x86_64", "x64")
    end)

    on_load(function (toolchain)
        import("lib.detect.find_tool")

        toolchain:set("toolset", "cc", "cl.exe")
        toolchain:set("toolset", "cxx", "cl.exe")
        toolchain:set("toolset", "ld", "link.exe")
        toolchain:set("toolset", "sh", "link.exe")
        toolchain:set("toolset", "ar", "lib.exe")

        local ccache = find_tool("ccache")
        if ccache then
            toolchain:set("toolset", "ccache", ccache.program)
        end

        local base_cflags = {
            "/arch:AVX2", "/EHsc", "/utf-8", "/GS",
            "/guard:cf", "/wd4244", "/wd4267", "/wd4834",
            "/permissive-",
        }

        toolchain:add("cflags", table.unpack(base_cflags))
        toolchain:add("cxflags", table.unpack(base_cflags))
        toolchain:add("defines", "_USE_MATH_DEFINES")

        local shared = is_config("boyle_build_shared", true)

        if is_mode("debug") then
            toolchain:add("cflags", "/Od", "/fsanitize=address", "/Zi")
            toolchain:add("cxflags", "/Od", "/fsanitize=address", "/Zi")
            toolchain:set("runtimes", shared and "MDd" or "MTd")
        elseif is_mode("release") then
            toolchain:add("cflags", "/O2", "/DNDEBUG", "/GL")
            toolchain:add("cxflags", "/O2", "/DNDEBUG", "/GL")
            toolchain:add("ldflags", "/LTCG")
            toolchain:add("shflags", "/LTCG")
            toolchain:set("runtimes", shared and "MD" or "MT")
        elseif is_mode("releasedbg") then
            toolchain:add("cflags", "/O2", "/DNDEBUG", "/fsanitize=address", "/Zi", "/GL")
            toolchain:add("cxflags", "/O2", "/DNDEBUG", "/fsanitize=address", "/Zi", "/GL")
            toolchain:add("ldflags", "/LTCG")
            toolchain:add("shflags", "/LTCG")
            toolchain:set("runtimes", shared and "MD" or "MT")
        elseif is_mode("minsizerel") then
            toolchain:add("cflags", "/O1", "/DNDEBUG", "/GL")
            toolchain:add("cxflags", "/O1", "/DNDEBUG", "/GL")
            toolchain:add("ldflags", "/LTCG")
            toolchain:add("shflags", "/LTCG")
            toolchain:set("runtimes", shared and "MD" or "MT")
        end
    end)
toolchain_end()
