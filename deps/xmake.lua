add_requires("openmp")

if is_plat("linux") then
    add_syslinks("pthread")
end

includes("boost/xmake.lua")
includes("cxxopts/xmake.lua")
includes("spdlog/xmake.lua")
includes("doctest/xmake.lua")
includes("msft_proxy4/xmake.lua")
includes("pocketfft/xmake.lua")
includes("qdldl/xmake.lua")
includes("osqp/xmake.lua")
includes("matplotplusplus/xmake.lua")
includes("openblas/xmake.lua")
