/**
 * @file border.hpp
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

#include <cstdint>
#include <limits>
#include <vector>

#include "zpp_bits.h"

#include "boyle/bicycle/planners/dualism.hpp"
#include "boyle/math/dense/vec2.hpp"

namespace boyle::bicycle {

template <::boyle::math::Vec2Arithmetic T>
struct HardBorder final {
    using serialize = zpp::bits::members<3>;

    using value_type = T;
    std::uint64_t id{std::numeric_limits<std::uint64_t>::quiet_NaN()};
    ::boyle::bicycle::Chirality chirality{::boyle::bicycle::Chirality::LEFT};
    std::pmr::vector<value_type> bound_points;
};

template <::boyle::math::Vec2Arithmetic T>
struct SoftBorder final {
    using serialize = zpp::bits::members<5>;

    using value_type = T;
    std::uint64_t id{std::numeric_limits<std::uint64_t>::quiet_NaN()};
    ::boyle::bicycle::Chirality chirality{::boyle::bicycle::Chirality::LEFT};
    std::pmr::vector<value_type> bound_points;
    typename value_type::value_type linear_weight{0.0};
    typename value_type::value_type quadratic_weight{0.0};
};

using HardBorder2s = HardBorder<::boyle::math::Vec2s>;

using HardBorder2d = HardBorder<::boyle::math::Vec2d>;

using SoftBorder2s = SoftBorder<::boyle::math::Vec2s>;

using SoftBorder2d = SoftBorder<::boyle::math::Vec2d>;

} // namespace boyle::bicycle
