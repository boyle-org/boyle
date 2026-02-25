/**
 * @file polygon.hpp
 * @author Houchen Li (houchen_li@hotmail.com)
 * @brief
 * @version 0.1
 * @date 2026-09-02
 *
 * @copyright Copyright (c) 2026 Boyle Development Team.
 *            All rights reserved.
 *
 */

#pragma once

#include <algorithm>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <format>
#include <limits>
#include <ranges>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include "zpp_bits.h"

#include "boyle/common/aligned_allocator.hpp"
#include "boyle/math/concepts.hpp"
#include "boyle/math/curves/piecewise_linear_curve.hpp"
#include "boyle/math/dense/vec2.hpp"
#include "boyle/math/utils.hpp"

namespace boyle::math {

template <
    ::boyle::math::Vec2Arithmetic T,
    ::boyle::math::Allocatory Alloc = ::boyle::common::AlignedAllocator<T>>
    requires std::floating_point<typename T::value_type>
class Polygon final {
  public:
    using value_type = T;
    using param_type = typename value_type::value_type;
    using curve_type = ::boyle::math::PiecewiseLinearCurve<value_type, Alloc>;
    using size_type = std::size_t;
    using allocator_type = Alloc;

    static constexpr param_type kDuplicateCriterion{1E-8};

    Polygon() noexcept = default;
    Polygon(const Polygon& other) = default;
    auto operator=(const Polygon& other) -> Polygon& = default;
    Polygon(Polygon&& other) noexcept = default;
    auto operator=(Polygon&& other) noexcept -> Polygon& = default;
    ~Polygon() noexcept = default;

    template <std::ranges::input_range R = std::initializer_list<value_type>>
    [[using gnu: ]]
    explicit Polygon(R&& vertices, const allocator_type& alloc = {})
        requires std::same_as<std::ranges::range_value_t<R>, value_type>
    {
#if BOYLE_CHECK_PARAMS == 1
        if (vertices.size() < 4) [[unlikely]] {
            throw std::invalid_argument(
                std::format(
                    "Invalid arguments detected! sizes of vertices must be greater than 4 (3 "
                    "distinct vertices plus one closing point equal to the first): "
                    "vertices.size() = {0:d}",
                    vertices.size()
                )
            );
        }
        if (hasDuplicates(vertices.begin(), std::prev(vertices.end()), kDuplicateCriterion))
            [[unlikely]] {
            throw std::invalid_argument(
                std::format(
                    "Invalid arguments detected! vertices can not have duplicated elements other "
                    "than the closing point!"
                )
            );
        }
#endif
        m_curve = curve_type(periodic_tag{}, std::forward<R>(vertices), 0.0, alloc);
    }

    [[using gnu: pure, always_inline]]
    auto eval(param_type s) const noexcept -> value_type {
        return m_curve.eval(s);
    }

    [[using gnu: pure]]
    auto signedArea() const noexcept -> param_type {
        const auto vs{vertices()};
        const size_type size{vs.size()};
        param_type area{0.0};
        for (size_type i{1}; i < size; ++i) {
            area += vs[i - 1].crossProj(vs[i]);
        }
        return area * static_cast<param_type>(0.5);
    }

    [[using gnu: pure, flatten, leaf, hot]]
    auto contains(const value_type& point) const noexcept -> bool {
        const auto vs{vertices()};
        const size_type size{vs.size()};
        size_type count{0};
        for (size_type i{1}; i < size; ++i) {
            const value_type edge{vs[i] - vs[i - 1]};
            const param_type edge_length{edge.euclidean()};
            if (edge_length < kEpsilon) {
                continue;
            }
            const value_type r{point - vs[i - 1]};
            if (const param_type ratio{std::isfinite(r.x / edge.x) ? r.x / edge.x : r.y / edge.y};
                inRange(ratio, 0.0 - kEpsilon, 1.0 + kEpsilon)) {
                if (std::abs(r.crossProj(edge)) / edge_length < kEpsilon) {
                    return true;
                }
                count += static_cast<size_type>(r.y < ratio * edge.y);
            }
        }
        return static_cast<bool>(count % 2);
    }

    [[using gnu: pure, always_inline]]
    auto operator()(param_type s) const noexcept -> value_type {
        return eval(s);
    }

    [[using gnu: pure, always_inline]]
    auto vertices() const noexcept -> std::span<const value_type> {
        return m_curve.anchorPoints();
    }

  private:
    friend zpp::bits::access;
    using serialize = zpp::bits::members<1>;

    curve_type m_curve;
};

using Polygon2s = Polygon<Vec2s>;
using Polygon2d = Polygon<Vec2d>;

template <
    ::boyle::math::Vec2Arithmetic T,
    ::boyle::math::Allocatory Alloc = ::boyle::common::AlignedAllocator<T>>
[[using gnu: pure]]
inline auto isConvex(const Polygon<T, Alloc>& obj) noexcept -> bool
    requires std::floating_point<typename T::value_type>
{
    using value_type = T;
    using param_type = typename T::value_type;

    const auto vs{obj.vertices()};
    const std::size_t size{vs.size()};
    int sign{0};
    for (std::size_t i{1}; i < size; ++i) {
        const value_type left_edge{vs[i] - vs[i - 1]};
        const value_type right_edge{i == size - 1 ? vs[1] - vs[0] : vs[i + 1] - vs[i]};
        const param_type cross{left_edge.crossProj(right_edge)};
        if (std::abs(cross) < kEpsilon) [[unlikely]] {
            continue;
        }
        const int curr_sign{cross > 0.0 ? 1 : -1};
        if (sign == 0) {
            sign = curr_sign;
        } else if (curr_sign != sign) {
            return false;
        }
    }
    return true;
}

template <
    ::boyle::math::Vec2Arithmetic T,
    ::boyle::math::Allocatory Alloc = ::boyle::common::AlignedAllocator<T>>
[[using gnu: pure]]
inline auto isConcave(const Polygon<T, Alloc>& obj) noexcept -> bool
    requires std::floating_point<typename T::value_type>
{
    return !isConvex(obj);
}

} // namespace boyle::math
