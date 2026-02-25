add_requires("spdlog 1.17.0", {configs = {
    std_format = true,
    shared = is_config("boyle_build_shared", true),
}})
