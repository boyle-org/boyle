/**
 * @file fence.hpp
 * @author Houchen Li (houchen_li@hotmail.com)
 * @brief
 * @version 0.1
 * @date 2023-09-06
 *
 * @copyright Copyright (c) 2023 Boyle Development Team
 *            All rights reserved.
 *
 */

#pragma once

#include <concepts>
#include <cstdint>
#include <limits>
#include <vector>

#include "zpp_bits.h"

#include "boyle/bicycle/planners/dualism.hpp"
#include "boyle/math/concepts.hpp"
#include "boyle/math/dense/detail/dense_degenerate_trait.hpp"
#include "boyle/math/dense/vec2.hpp"

namespace boyle::bicycle {

template <typename T>
    requires std::floating_point<T> || ::boyle::math::VecArithmetic<T>
struct HardFence final {
    using serialize = zpp::bits::members<4>;

    using value_type = T;
    using param_type = ::boyle::math::detail::DenseDegenerateTraitT<value_type>;
    std::uint64_t id{std::numeric_limits<std::uint64_t>::quiet_NaN()};
    ::boyle::bicycle::Actio actio{::boyle::bicycle::Actio::BLOCKING};
    std::pmr::vector<param_type> bound_ts;
    std::pmr::vector<value_type> bound_ss;
};

template <typename T>
    requires std::floating_point<T> || ::boyle::math::VecArithmetic<T>
struct SoftFence final {
    using serialize = zpp::bits::members<6>;

    using value_type = T;
    using param_type = ::boyle::math::detail::DenseDegenerateTraitT<value_type>;
    std::uint64_t id{std::numeric_limits<std::uint64_t>::quiet_NaN()};
    ::boyle::bicycle::Actio actio{::boyle::bicycle::Actio::BLOCKING};
    std::pmr::vector<param_type> bound_ts;
    std::pmr::vector<value_type> bound_ss;
    param_type linear_weight{0.0};
    param_type quadratic_weight{0.0};
};

using HardFence1s = HardFence<float>;

using HardFence1d = HardFence<double>;

using SoftFence1s = SoftFence<float>;

using SoftFence1d = SoftFence<double>;

using HardFence2s = HardFence<::boyle::math::Vec2s>;

using HardFence2d = HardFence<::boyle::math::Vec2d>;

using SoftFence2s = SoftFence<::boyle::math::Vec2s>;

using SoftFence2d = SoftFence<::boyle::math::Vec2d>;

} // namespace boyle::bicycle
