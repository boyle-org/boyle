toolchain("linux-clang-x64")
    set_kind("standalone")
    set_homepage("https://llvm.org/")
    set_description("Linux Clang x86_64-pc-linux-gnu")

    on_check(function (toolchain)
        return is_host("linux") and is_arch("x86_64")
    end)

    on_load(function (toolchain)
        import("lib.detect.find_tool")

        toolchain:set("toolset", "cc", "clang")
        toolchain:set("toolset", "cxx", "clang++")
        toolchain:set("toolset", "fc", "flang")
        toolchain:set("toolset", "ld", "clang++")
        toolchain:set("toolset", "sh", "clang++")
        toolchain:set("toolset", "ar", "llvm-ar")
        toolchain:set("toolset", "ranlib", "llvm-ranlib")
        toolchain:set("toolset", "as", "clang")

        local ccache = find_tool("ccache")
        if ccache then
            toolchain:set("toolset", "ccache", ccache.program)
        end

        local lld = find_tool("ld.lld")
        if lld then
            toolchain:add("ldflags", "-fuse-ld=lld")
            toolchain:add("shflags", "-fuse-ld=lld")
        end

        local base_cflags = {
            "-march=x86-64-v3", "-mtune=generic", "-pipe", "-fexceptions",
            "-fno-omit-frame-pointer", "-mno-omit-leaf-frame-pointer",
            "-Wall", "-Wextra", "-Wpedantic",
        }
        local base_cxxflags = {"-stdlib=libc++"}
        local base_fcflags = {"-march=x86-64-v3", "-mtune=generic", "-fno-omit-frame-pointer"}
        local base_ldflags = {
            "-stdlib=libc++", "-rtlib=compiler-rt",
            "--unwindlib=libunwind", "-lc++abi",
        }

        toolchain:add("cflags", table.unpack(base_cflags))
        toolchain:add("cxflags", table.unpack(base_cflags))
        toolchain:add("cxxflags", table.unpack(base_cxxflags))
        toolchain:add("fcflags", table.unpack(base_fcflags))
        toolchain:add("ldflags", table.unpack(base_ldflags))
        toolchain:add("shflags", table.unpack(base_ldflags))

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
            toolchain:add("cflags", "-flto=thin")
            toolchain:add("cxflags", "-flto=thin")
            toolchain:add("fcflags", "-flto=thin")
            toolchain:add("ldflags", "-flto=thin")
            toolchain:add("shflags", "-flto=thin")
        elseif is_mode("releasedbg") then
            local relwithdebinfo_flags = {"-O2", "-g", "-DNDEBUG", "-Wp,-D_FORTIFY_SOURCE=3", "-fsanitize=address,undefined", "-ftree-vectorize"}
            toolchain:add("cflags", table.unpack(relwithdebinfo_flags))
            toolchain:add("cxflags", table.unpack(relwithdebinfo_flags))
            toolchain:add("fcflags", "-O2", "-g", "-DNDEBUG", "-ftree-vectorize")
            toolchain:add("ldflags", "-fsanitize=address,undefined", "-flto=thin")
            toolchain:add("shflags", "-fsanitize=address,undefined", "-flto=thin")
            toolchain:add("cflags", "-flto=thin")
            toolchain:add("cxflags", "-flto=thin")
            toolchain:add("fcflags", "-flto=thin")
        elseif is_mode("minsizerel") then
            local minsizerel_flags = {"-Os", "-DNDEBUG", "-Wp,-D_FORTIFY_SOURCE=3", "-ftree-vectorize"}
            toolchain:add("cflags", table.unpack(minsizerel_flags))
            toolchain:add("cxflags", table.unpack(minsizerel_flags))
            toolchain:add("fcflags", "-Os", "-DNDEBUG", "-ftree-vectorize")
            toolchain:add("cflags", "-flto=thin")
            toolchain:add("cxflags", "-flto=thin")
            toolchain:add("fcflags", "-flto=thin")
            toolchain:add("ldflags", "-flto=thin")
            toolchain:add("shflags", "-flto=thin")
        end
    end)
toolchain_end()
