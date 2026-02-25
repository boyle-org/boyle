function install(package)
    import("package.tools.make")

    local cc = package:tool("cc") or ""
    local fc = package:tool("fc") or ""
    local is_clang_cc = cc:find("clang", 1, true) ~= nil

    local function get_env_flags(name)
        local val = package:build_getenv(name)
        if type(val) == "table" then
            return table.concat(val, " ")
        end
        return val or ""
    end

    local cflags = get_env_flags("cflags")
    local fflags = get_env_flags("fflags")
    -- Darwin: no LTO; Clang needs compiler-rt to resolve builtins
    local ldflags = is_clang_cc and "-rtlib=compiler-rt" or ""

    local configs = {
        "AR="       .. (package:tool("ar")     or "ar"),
        "RANLIB="   .. (package:tool("ranlib") or "ranlib"),
        "CC="       .. cc,
        "FC="       .. fc,
        "ASMFLAGS=" .. cflags,
        "CFLAGS="   .. cflags,
        "FFLAGS="   .. fflags,
        "LDFLAGS="  .. ldflags,
        "TARGET="   .. (package:is_arch("x86_64") and "HASWELL" or "ARMV8"),
        "DYNAMIC_ARCH=0",
        "NO_AFFINITY=0",
        "USE_THREAD=0",
        "USE_LOCKING=0",
        "USE_OPENMP=0",
        "NO_SHARED=1",
    }
    make.build(package, configs)
    make.make(package, table.join("install", "PREFIX=" .. package:installdir(), configs))
end
