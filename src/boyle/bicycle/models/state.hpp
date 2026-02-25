/**
 * @file state.hpp
 * @author Houchen Li (houchen_li@hotmail.com)
 * @brief
 * @version 0.1
 * @date 2026-08-08
 *
 * @copyright Copyright (c) 2026 Boyle Development Team.
 *            All rights reserved.
 *
 */

#pragma once

#include <cmath>
#include <concepts>
#include <cstddef>
#include <type_traits>
#include <utility>

#include "zpp_bits.h"

namespace boyle::bicycle {

template <std::floating_point T>
struct State final {
    using value_type = T;
    using pointer = std::add_pointer_t<value_type>;
    using const_pointer = std::add_pointer_t<std::add_const_t<value_type>>;
    using size_type = std::size_t;

    static constexpr size_type kSize = 6;

    [[using gnu: const, always_inline, leaf]]
    static constexpr auto size() noexcept -> size_type {
        return kSize;
    }

    [[using gnu: pure, always_inline]]
    constexpr auto data() noexcept -> pointer {
        return &x;
    }

    [[using gnu: pure, always_inline]]
    constexpr auto data() const noexcept -> const_pointer {
        return &x;
    }

    [[using gnu: pure, always_inline]]
    auto speed() const noexcept -> value_type {
        const value_type projection{dx * std::cos(yaw) + dy * std::sin(yaw)};
        const value_type magnitude{std::hypot(dx, dy)};
        return projection < 0.0 ? -magnitude : magnitude;
    }

    [[using gnu: pure, always_inline]]
    constexpr auto operator==(const State& obj) const noexcept -> bool = default;

    [[using gnu: always_inline, leaf]]
    constexpr auto operator+=(const State& obj) noexcept -> State& {
        x += obj.x;
        y += obj.y;
        yaw += obj.yaw;
        dx += obj.dx;
        dy += obj.dy;
        dyaw += obj.dyaw;
        return *this;
    }

    [[using gnu: pure, always_inline]]
    constexpr auto operator+(const State& obj) const& noexcept -> State {
        State result{*this};
        result += obj;
        return result;
    }

    [[using gnu: always_inline]]
    constexpr auto operator+(State&& obj) const& noexcept -> State&& {
        obj += *this;
        return std::move(obj);
    }

    [[using gnu: always_inline]]
    constexpr auto operator+(const State& obj) && noexcept -> State&& {
        operator+=(obj);
        return std::move(*this);
    }

    [[using gnu: always_inline]]
    constexpr auto operator+(State&& obj) && noexcept -> State&& {
        operator+=(obj);
        return std::move(*this);
    }

    [[using gnu: always_inline, leaf]]
    constexpr auto operator-=(const State& obj) noexcept -> State& {
        x -= obj.x;
        y -= obj.y;
        yaw -= obj.yaw;
        dx -= obj.dx;
        dy -= obj.dy;
        dyaw -= obj.dyaw;
        return *this;
    }

    [[using gnu: pure, always_inline]]
    constexpr auto operator-(const State& obj) const& noexcept -> State {
        State result{*this};
        result -= obj;
        return result;
    }

    [[using gnu: always_inline]]
    constexpr auto operator-(State&& obj) const& noexcept -> State&& {
        obj *= -1.0;
        obj += *this;
        return std::move(obj);
    }

    [[using gnu: always_inline]]
    constexpr auto operator-(const State& obj) && noexcept -> State&& {
        operator-=(obj);
        return std::move(*this);
    }

    [[using gnu: always_inline]]
    constexpr auto operator-(State&& obj) && noexcept -> State&& {
        operator-=(obj);
        return std::move(*this);
    }

    [[using gnu: always_inline, leaf]]
    constexpr auto operator*=(const std::floating_point auto& fac) noexcept -> State& {
        const auto alpha{static_cast<value_type>(fac)};
        x *= alpha;
        y *= alpha;
        yaw *= alpha;
        dx *= alpha;
        dy *= alpha;
        dyaw *= alpha;
        return *this;
    }

    [[using gnu: pure, always_inline]]
    constexpr auto operator*(const std::floating_point auto& fac) const& noexcept -> State {
        State result{*this};
        result *= fac;
        return result;
    }

    [[using gnu: always_inline]]
    constexpr auto operator*(const std::floating_point auto& fac) && noexcept -> State&& {
        operator*=(fac);
        return std::move(*this);
    }

    [[using gnu: always_inline, leaf]]
    constexpr auto operator/=(const std::floating_point auto& den) noexcept -> State& {
        const auto alpha{static_cast<value_type>(1.0) / static_cast<value_type>(den)};
        operator*=(alpha);
        return *this;
    }

    [[using gnu: pure, always_inline]]
    constexpr auto operator/(const std::floating_point auto& den) const& noexcept -> State {
        State result{*this};
        result /= den;
        return result;
    }

    [[using gnu: always_inline]]
    constexpr auto operator/(const std::floating_point auto& den) && noexcept -> State&& {
        operator/=(den);
        return std::move(*this);
    }

    [[using gnu: pure, always_inline]]
    constexpr auto operator-() const& noexcept -> State {
        State result{*this};
        result *= -1.0;
        return result;
    }

    [[using gnu: always_inline, hot]]
    constexpr auto operator-() && noexcept -> State&& {
        operator*=(-1.0);
        return std::move(*this);
    }

    using serialize = zpp::bits::members<kSize>;

    value_type x{0.0};
    value_type y{0.0};
    value_type yaw{0.0};
    value_type dx{0.0};
    value_type dy{0.0};
    value_type dyaw{0.0};
};

template <std::floating_point T>
[[using gnu: pure, always_inline, hot]]
inline constexpr auto operator*(const std::floating_point auto& fac, const State<T>& obj) noexcept
    -> State<T> {
    return obj * fac;
}

template <std::floating_point T>
[[using gnu: always_inline, hot]]
inline constexpr auto operator*(const std::floating_point auto& fac, State<T>&& obj) noexcept
    -> State<T>&& {
    obj *= fac;
    return std::move(obj);
}

} // namespace boyle::bicycle
