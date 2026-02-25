add_requires("boost 1.90.0", {configs = {
    cmake = true,
    asio = true,
    interprocess = true,
    multiprecision = true,
    system = true,
    shared = is_config("kind", "shared")
}})
