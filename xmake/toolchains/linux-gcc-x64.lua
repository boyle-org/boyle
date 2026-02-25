toolchain("linux-gcc-x64")
    set_kind("standalone")
    set_homepage("https://gcc.gnu.org/")
    set_description("Linux GCC x86_64-pc-linux-gnu")

    on_check(function (toolchain)
        return is_host("linux") and is_arch("x86_64")
    end)

    on_load(function (toolchain)
        import("lib.detect.find_tool")

        toolchain:set("toolset", "cc", "gcc")
        toolchain:set("toolset", "cxx", "g++")
        toolchain:set("toolset", "fc", "gfortran")
        toolchain:set("toolset", "ld", "g++")
        toolchain:set("toolset", "sh", "g++")
        toolchain:set("toolset", "ar", "ar")
        toolchain:set("toolset", "as", "gcc")

        local ccache = find_tool("ccache")
        if ccache then
            toolchain:set("toolset", "ccache", ccache.program)
        end

        local bfd = find_tool("ld.bfd")
        if bfd then
            toolchain:add("ldflags", "-fuse-ld=bfd")
            toolchain:add("shflags", "-fuse-ld=bfd")
        end

        local base_cflags = {
            "-march=x86-64-v3", "-mtune=generic", "-pipe", "-fno-plt", "-fexceptions",
            "-fstack-clash-protection", "-fcf-protection",
            "-fno-omit-frame-pointer", "-mno-omit-leaf-frame-pointer",
            "-Wall", "-Wextra", "-Wpedantic", "-Wno-maybe-uninitialized",
        }
        local base_cxxflags = {"-Wp,-D_GLIBCXX_ASSERTIONS"}
        local base_fcflags = {"-march=x86-64-v3", "-mtune=generic", "-fno-omit-frame-pointer", "-frecursive"}

        toolchain:add("cflags", table.unpack(base_cflags))
        toolchain:add("cxflags", table.unpack(base_cflags))
        toolchain:add("cxxflags", table.unpack(base_cxxflags))
        toolchain:add("fcflags", table.unpack(base_fcflags))

        if is_mode("debug") then
            local debug_flags = {"-O0", "-g", "-fsanitize=address,undefined"}
            toolchain:add("cflags", table.unpack(debug_flags))
            toolchain:add("cxflags", table.unpack(debug_flags))
            toolchain:add("fcflags", "-O0", "-g")
            toolchain:add("ldflags", "-fsanitize=address,undefined")
            toolchain:add("shflags", "-fsanitize=address,undefined")
        elseif is_mode("release") then
            local release_flags = {"-O2", "-DNDEBUG", "-Wp,-D_FORTIFY_SOURCE=3", "-ftree-vectorize"}
            toolchain:add("cflags", table.unpack(release_flags))
            toolchain:add("cxflags", table.unpack(release_flags))
            toolchain:add("fcflags", "-O2", "-DNDEBUG", "-ftree-vectorize")
            toolchain:add("cflags", "-flto")
            toolchain:add("cxflags", "-flto")
            toolchain:add("fcflags", "-flto")
            toolchain:add("ldflags", "-flto")
            toolchain:add("shflags", "-flto")
        elseif is_mode("releasedbg") then
            local relwithdebinfo_flags = {"-O2", "-g", "-DNDEBUG", "-Wp,-D_FORTIFY_SOURCE=3", "-fsanitize=address,undefined", "-ftree-vectorize"}
            toolchain:add("cflags", table.unpack(relwithdebinfo_flags))
            toolchain:add("cxflags", table.unpack(relwithdebinfo_flags))
            toolchain:add("fcflags", "-O2", "-g", "-DNDEBUG", "-ftree-vectorize")
            toolchain:add("ldflags", "-fsanitize=address,undefined", "-flto")
            toolchain:add("shflags", "-fsanitize=address,undefined", "-flto")
            toolchain:add("cflags", "-flto")
            toolchain:add("cxflags", "-flto")
            toolchain:add("fcflags", "-flto")
        elseif is_mode("minsizerel") then
            local minsizerel_flags = {"-Os", "-DNDEBUG", "-Wp,-D_FORTIFY_SOURCE=3", "-ftree-vectorize"}
            toolchain:add("cflags", table.unpack(minsizerel_flags))
            toolchain:add("cxflags", table.unpack(minsizerel_flags))
            toolchain:add("fcflags", "-Os", "-DNDEBUG", "-ftree-vectorize")
            toolchain:add("cflags", "-flto")
            toolchain:add("cxflags", "-flto")
            toolchain:add("fcflags", "-flto")
            toolchain:add("ldflags", "-flto")
            toolchain:add("shflags", "-flto")
        end
    end)
toolchain_end()
