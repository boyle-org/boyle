/**
 * @file kinematics_model.hpp
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
#include <format>
#include <numbers>
#include <span>
#include <stdexcept>
#include <utility>

#include "zpp_bits.h"

#include "boyle/bicycle/models/state.hpp"

namespace boyle::bicycle {

template <std::floating_point T>
class KinematicsModel final {
  public:
    using value_type = T;
    using state_type = State<value_type>;

    KinematicsModel() noexcept = default;
    KinematicsModel(const KinematicsModel& other) noexcept = default;
    auto operator=(const KinematicsModel& other) noexcept -> KinematicsModel& = default;
    KinematicsModel(KinematicsModel&& other) noexcept = default;
    auto operator=(KinematicsModel&& other) noexcept -> KinematicsModel& = default;
    ~KinematicsModel() noexcept = default;

    explicit KinematicsModel(value_type front_wheelbase, value_type rear_wheelbase) noexcept(
        !BOYLE_CHECK_PARAMS
    )
        : m_front_wheelbase{front_wheelbase}, m_rear_wheelbase{rear_wheelbase} {
#if BOYLE_CHECK_PARAMS == 1
        if (front_wheelbase <= 0.0 || rear_wheelbase <= 0.0) [[unlikely]] {
            throw std::invalid_argument(
                std::format(
                    "Invalid arguments detected! boyle::bicycle::KinematicsModel requires both "
                    "wheelbases to be positive: front_wheelbase = {0:f} while rear_wheelbase = "
                    "{1:f}.",
                    front_wheelbase, rear_wheelbase
                )
            );
        }
#endif
    }

    [[using gnu: pure, always_inline]]
    constexpr auto front_wheelbase() const noexcept -> value_type {
        return m_front_wheelbase;
    }

    [[using gnu: pure, always_inline]]
    constexpr auto rear_wheelbase() const noexcept -> value_type {
        return m_rear_wheelbase;
    }

    [[using gnu: pure, always_inline]]
    constexpr auto wheelbase() const noexcept -> value_type {
        return m_front_wheelbase + m_rear_wheelbase;
    }

    auto createState(
        value_type x, value_type y, value_type yaw, value_type speed, value_type steering_angle
    ) const noexcept(!BOYLE_CHECK_PARAMS) -> state_type {
#if BOYLE_CHECK_PARAMS == 1
        if (std::abs(steering_angle) >= std::numbers::pi_v<value_type> * 0.5) [[unlikely]] {
            throw std::invalid_argument(
                std::format(
                    "Invalid arguments detected! boyle::bicycle::KinematicsModel::createState() "
                    "requires the steering_angle to lie in (-pi/2, pi/2): steering_angle = {0:f}.",
                    steering_angle
                )
            );
        }
#endif
        const value_type course{yaw + slipAngle(steering_angle)};
        return state_type{
            .x{x},
            .y{y},
            .yaw{yaw},
            .dx{speed * std::cos(course)},
            .dy{speed * std::sin(course)},
            .dyaw{curvature(steering_angle) * speed}
        };
    }

    [[using gnu: always_inline]]
    auto forwardEuler(
        state_type initial_state, value_type h, value_type accel, value_type steering_angle
    ) const noexcept(!BOYLE_CHECK_PARAMS) -> state_type {
#if BOYLE_CHECK_PARAMS == 1
        if (h <= 0.0) [[unlikely]] {
            throw std::invalid_argument(
                std::format(
                    "Invalid arguments detected! boyle::bicycle::KinematicsModel::forwardEuler() "
                    "requires a "
                    "positive time step: h = {0:f}.",
                    h
                )
            );
        }
        if (!(std::abs(steering_angle) < std::numbers::pi_v<value_type> * 0.5)) [[unlikely]] {
            throw std::invalid_argument(
                std::format(
                    "Invalid arguments detected! boyle::bicycle::KinematicsModel::forwardEuler() "
                    "requires "
                    "the steering_angle to lie in (-pi/2, pi/2): steering_angle = {0:f}.",
                    steering_angle
                )
            );
        }
#endif
        initial_state += tangent(initial_state, accel, steering_angle) * h;
        return initial_state;
    }

    [[using gnu: always_inline]]
    auto forwardEuler(
        state_type initial_state, std::span<const value_type> hs,
        std::span<const value_type> accels, std::span<const value_type> steering_angles
    ) const noexcept(!BOYLE_CHECK_PARAMS) -> state_type {
#if BOYLE_CHECK_PARAMS == 1
        if (hs.size() != accels.size() || hs.size() != steering_angles.size()) [[unlikely]] {
            throw std::invalid_argument(
                std::format(
                    "Invalid arguments detected! boyle::bicycle::KinematicsModel::forwardEuler() "
                    "requires "
                    "the three input spans to have identical sizes: hs.size() = {0:d}, "
                    "accels.size() = {1:d}, steering_angles.size() = {2:d}.",
                    hs.size(), accels.size(), steering_angles.size()
                )
            );
        }
#endif
        const std::size_t size{hs.size()};
        for (std::size_t i{0}; i < size; ++i) {
            initial_state =
                forwardEuler(std::move(initial_state), hs[i], accels[i], steering_angles[i]);
        }
        return initial_state;
    }

    [[using gnu: always_inline]]
    auto symplecticEuler(
        state_type initial_state, value_type h, value_type accel, value_type steering_angle
    ) const noexcept(!BOYLE_CHECK_PARAMS) -> state_type {
#if BOYLE_CHECK_PARAMS == 1
        if (h <= 0.0) [[unlikely]] {
            throw std::invalid_argument(
                std::format(
                    "Invalid arguments detected! "
                    "boyle::bicycle::KinematicsModel::symplecticEuler() requires a positive time "
                    "step: h = {0:f}.",
                    h
                )
            );
        }
        if (!(std::abs(steering_angle) < std::numbers::pi_v<value_type> * 0.5)) [[unlikely]] {
            throw std::invalid_argument(
                std::format(
                    "Invalid arguments detected! "
                    "boyle::bicycle::KinematicsModel::symplecticEuler() requires the "
                    "steering_angle to lie in (-pi/2, pi/2): steering_angle = {0:f}.",
                    steering_angle
                )
            );
        }
#endif
        const state_type k{tangent(initial_state, accel, steering_angle) * h};
        const value_type dx{initial_state.dx + k.dx};
        const value_type dy{initial_state.dy + k.dy};
        const value_type dyaw{initial_state.dyaw + k.dyaw};
        return state_type{
            .x{initial_state.x + dx * h},
            .y{initial_state.y + dy * h},
            .yaw{initial_state.yaw + dyaw * h},
            .dx{dx},
            .dy{dy},
            .dyaw{dyaw}
        };
    }

    [[using gnu: always_inline]]
    auto symplecticEuler(
        state_type initial_state, std::span<const value_type> hs,
        std::span<const value_type> accels, std::span<const value_type> steering_angles
    ) const noexcept(!BOYLE_CHECK_PARAMS) -> state_type {
#if BOYLE_CHECK_PARAMS == 1
        if (hs.size() != accels.size() || hs.size() != steering_angles.size()) [[unlikely]] {
            throw std::invalid_argument(
                std::format(
                    "Invalid arguments detected! "
                    "boyle::bicycle::KinematicsModel::symplecticEuler() requires the three input "
                    "spans to have identical sizes: hs.size() = {0:d}, accels.size() = {1:d}, "
                    "steering_angles.size() = {2:d}.",
                    hs.size(), accels.size(), steering_angles.size()
                )
            );
        }
#endif
        const std::size_t size{hs.size()};
        for (std::size_t i{0}; i < size; ++i) {
            initial_state =
                symplecticEuler(std::move(initial_state), hs[i], accels[i], steering_angles[i]);
        }
        return initial_state;
    }

    [[using gnu: always_inline]]
    auto rungeKutta(
        state_type initial_state, value_type h, value_type accel, value_type steering_angle
    ) const noexcept(!BOYLE_CHECK_PARAMS) -> state_type {
#if BOYLE_CHECK_PARAMS == 1
        if (h <= 0.0) [[unlikely]] {
            throw std::invalid_argument(
                std::format(
                    "Invalid arguments detected! boyle::bicycle::KinematicsModel::rungeKutta() "
                    "requires a "
                    "positive time step: h = {0:f}.",
                    h
                )
            );
        }
        if (!(std::abs(steering_angle) < std::numbers::pi_v<value_type> * 0.5)) [[unlikely]] {
            throw std::invalid_argument(
                std::format(
                    "Invalid arguments detected! boyle::bicycle::KinematicsModel::rungeKutta() "
                    "requires "
                    "the steering_angle to lie in (-pi/2, pi/2): steering_angle = {0:f}.",
                    steering_angle
                )
            );
        }
#endif
        constexpr value_type kHalf{static_cast<value_type>(0.5)};
        constexpr value_type kSixth{static_cast<value_type>(1.0 / 6.0)};
        constexpr value_type kThird{static_cast<value_type>(1.0 / 3.0)};
        const state_type k1{tangent(initial_state, accel, steering_angle) * h};
        const state_type k2{tangent(initial_state + k1 * kHalf, accel, steering_angle) * h};
        const state_type k3{tangent(initial_state + k2 * kHalf, accel, steering_angle) * h};
        const state_type k4{tangent(initial_state + k3, accel, steering_angle) * h};
        initial_state += k1 * kSixth + k2 * kThird + k3 * kThird + k4 * kSixth;
        return initial_state;
    }

    [[using gnu: always_inline]]
    auto rungeKutta(
        state_type initial_state, std::span<const value_type> hs,
        std::span<const value_type> accels, std::span<const value_type> steering_angles
    ) const noexcept(!BOYLE_CHECK_PARAMS) -> state_type {
#if BOYLE_CHECK_PARAMS == 1
        if (hs.size() != accels.size() || hs.size() != steering_angles.size()) [[unlikely]] {
            throw std::invalid_argument(
                std::format(
                    "Invalid arguments detected! boyle::bicycle::KinematicsModel::rungeKutta() "
                    "requires "
                    "the three input spans to have identical sizes: hs.size() = {0:d}, "
                    "accels.size() = {1:d}, steering_angles.size() = {2:d}.",
                    hs.size(), accels.size(), steering_angles.size()
                )
            );
        }
#endif
        const std::size_t size{hs.size()};
        for (std::size_t i{0}; i < size; ++i) {
            initial_state =
                rungeKutta(std::move(initial_state), hs[i], accels[i], steering_angles[i]);
        }
        return initial_state;
    }

  private:
    /**
     * @brief Side slip angle at the center of gravity for the given steering angle.
     */
    [[using gnu: pure, always_inline]]
    auto slipAngle(value_type steering_angle) const noexcept -> value_type {
        return std::atan2(std::tan(steering_angle) * m_rear_wheelbase, wheelbase());
    }

    /**
     * @brief Curvature of the center of gravity path for the given steering angle.
     *
     * The path curvature does not depend on the speed, so a constant steering angle always traces
     * a circular arc of radius 1 / curvature() no matter how the vehicle accelerates along it.
     */
    [[using gnu: pure, always_inline]]
    auto curvature(value_type steering_angle) const noexcept -> value_type {
        return std::cos(slipAngle(steering_angle)) * std::tan(steering_angle) / wheelbase();
    }

    /**
     * @brief Tangent vector of the flow at the given state.
     *
     * State doubles as its own tangent space here, so the returned object is not a state: its
     * x and y fields hold the velocity, its dx and dy fields hold the acceleration, its yaw field
     * holds the yaw rate and its dyaw field holds the yaw acceleration. It is only meant to be
     * scaled and added onto a state.
     */
    [[using gnu: pure, always_inline]]
    auto tangent(
        const state_type& state, value_type accel, value_type steering_angle
    ) const noexcept -> state_type {
        const value_type speed{state.speed()};
        const value_type course{state.yaw + slipAngle(steering_angle)};
        const value_type dyaw{state.dyaw};
        return state_type{
            .x{state.dx},
            .y{state.dy},
            .yaw{dyaw},
            .dx{accel * std::cos(course) - speed * std::sin(course) * dyaw},
            .dy{accel * std::sin(course) + speed * std::cos(course) * dyaw},
            .dyaw{curvature(steering_angle) * accel}
        };
    }

    friend zpp::bits::access;
    using serialize = zpp::bits::members<2>;

    value_type m_front_wheelbase{1.0};
    value_type m_rear_wheelbase{1.0};
};

} // namespace boyle::bicycle
