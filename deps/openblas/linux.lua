function install(package)
    import("package.tools.make")

    local cc = package:tool("cc") or ""
    local fc = package:tool("fc") or ""
    local is_clang_cc = cc:find("clang", 1, true) ~= nil
    local is_flang_fc = fc:find("flang", 1, true) ~= nil

    local function get_env_flags(name)
        local val = package:build_getenv(name)
        if type(val) == "table" then
            return table.concat(val, " ")
        end
        return val or ""
    end

    local function join_flags(base, extra)
        if extra == "" then return base end
        if base == "" then return extra end
        return base .. " " .. extra
    end

    local cflags_base = get_env_flags("cflags")
    local fflags_base = get_env_flags("fflags")
    local lto_cflags, lto_fflags, ldflags

    if not is_clang_cc then
        -- GCC + GFortran
        lto_cflags = "-flto=auto -fno-fat-lto-objects"
        lto_fflags = "-flto=auto -fno-fat-lto-objects"
        ldflags    = "-fuse-ld=bfd"
    elseif is_flang_fc then
        -- Clang + LLVMFlang
        lto_cflags = "-flto=thin"
        lto_fflags = "-flto=thin"
        ldflags    = "-fuse-ld=lld -rtlib=compiler-rt"
    else
        -- Clang + GFortran: gfortran cannot emit LLVM bitcode, disable LTO on Fortran side
        lto_cflags = "-flto=thin"
        lto_fflags = "-fno-lto"
        ldflags    = "-fuse-ld=lld -rtlib=compiler-rt"
    end

    local cflags = join_flags(cflags_base, lto_cflags)
    local fflags = join_flags(fflags_base, lto_fflags)

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
