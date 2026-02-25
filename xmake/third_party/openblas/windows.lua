function load(package)
    package:add("links", "openblas")
    package:add("defines", "HAVE_LAPACK_CONFIG_H", "LAPACK_COMPLEX_CPP")
end

function _detect_target(package)
    import("core.project.config")
    import("core.tool.toolchain")
    local tc_name = config.get("toolchain")
    if not tc_name then
        raise("openblas: cannot determine active toolchain")
    end
    local tc = toolchain.load(tc_name, {plat = package:plat(), arch = package:arch()})
    local cxflags = tc:get("cxflags") or {}
    local cxflags_str = table.concat(cxflags, " ")
    if cxflags_str:find("/arch:AVX2", 1, true) then
        return "HASWELL"
    end
    raise("openblas: cannot detect a supported /arch in toolchain cxflags (expected /arch:AVX2)")
end

function install(package)
    local target = _detect_target(package)
    local configs = {
        "-DCMAKE_POLICY_VERSION_MINIMUM=3.5",
        "-DCMAKE_COMPILE_WARNING_AS_ERROR=OFF",
        "-DBUILD_TESTING=OFF",
        "-DNOFORTRAN=ON",
        "-DC_LAPACK=ON",
        "-DTARGET=" .. target,
        "-DDYNAMIC_ARCH=OFF",
        "-DUSE_THREAD=OFF",
        "-DUSE_OPENMP=OFF",
    }
    table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:is_debug() and "Debug" or "Release"))
    table.insert(configs, "-DBUILD_SHARED_LIBS=" .. (package:config("shared") and "ON" or "OFF"))
    import("package.tools.cmake").install(package, configs)
end
