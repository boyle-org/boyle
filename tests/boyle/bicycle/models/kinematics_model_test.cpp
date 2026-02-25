/**
 * @file kinematics_model_test.cpp
 * @author Houchen Li (houchen_li@hotmail.com)
 * @brief
 * @version 0.1
 * @date 2026-08-08
 *
 * @copyright Copyright (c) 2026 Boyle Development Team.
 *            All rights reserved.
 *
 */

#include "boyle/bicycle/models/kinematics_model.hpp"

#include <algorithm>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <numbers>
#include <span>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

#include "cxxopts.hpp"
#include "matplot/matplot.h"
#include "zpp_bits.h"

#include "boyle/math/dense/vec2.hpp"

#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest/doctest.h"

namespace {

bool plot_graph{false};

auto createFigureHandle() noexcept -> matplot::figure_handle {
    matplot::figure_handle fig = matplot::figure();
    fig->size(1600, 1000);
    return fig;
}

auto createAxesHandles(matplot::figure_handle& fig) noexcept -> std::vector<matplot::axes_handle> {
    std::vector<matplot::axes_handle> axes;
    axes.reserve(4);
    for (std::size_t i = 0; i < 4; ++i) {
        axes.emplace_back(fig->add_subplot(2, 2, i));
        axes[i]->grid(matplot::on);
        axes[i]->hold(matplot::on);
        axes[i]->font_size(12.0);
        axes[i]->x_axis().label_font_size(12.0);
        axes[i]->y_axis().label_font_size(12.0);
    }
    axes[0]->xlabel("x");
    axes[0]->ylabel("y");
    axes[1]->xlabel("t");
    axes[1]->ylabel("speed");
    axes[2]->xlabel("t");
    axes[2]->ylabel("yaw");
    axes[3]->xlabel("t");
    axes[3]->ylabel("position error");
    return axes;
}

} // namespace

namespace boyle::bicycle {

namespace {

template <std::floating_point T>
inline constexpr T kFrontWheelbase{static_cast<T>(1.2)};
template <std::floating_point T>
inline constexpr T kRearWheelbase{static_cast<T>(1.4)};
template <std::floating_point T>
inline constexpr T kInitialYaw{static_cast<T>(0.3)};

/**
 * @brief One sample of the analytic solution of the model.
 */
template <std::floating_point T>
struct Sample final {
    ::boyle::math::Vec2<T> center;
    T yaw;
    T speed;
};

/**
 * @brief Side slip angle written as beta = atan(l_r * tan(delta) / (l_f + l_r)).
 *
 * Deliberately spelled out from the literature rather than taken from State::slipAngle(), so that
 * the reference solution stays independent of the implementation under test.
 */
template <std::floating_point T>
[[nodiscard]] auto slipAngleOf(T steering_angle) noexcept -> T {
    return std::atan(
        kRearWheelbase<T> * std::tan(steering_angle) / (kFrontWheelbase<T> + kRearWheelbase<T>)
    );
}

/**
 * @brief Path curvature written as kappa = sin(beta) / l_r.
 *
 * This is the d(yaw)/dt = speed * sin(beta) / l_r form given by Kong et al., which is an algebraic
 * rearrangement of the cos(beta) * tan(delta) / wheelbase form the implementation uses.
 */
template <std::floating_point T>
[[nodiscard]] auto curvatureOf(T steering_angle) noexcept -> T {
    return std::sin(slipAngleOf(steering_angle)) / kRearWheelbase<T>;
}

/**
 * @brief Gathers the flat position and velocity fields of a state back into a Vec2.
 *
 * State stores its six scalars flat, while the reference solution below is written in terms of
 * planar vectors, so these two keep the assertions reading the way the geometry does.
 */
template <std::floating_point T>
[[nodiscard]] auto centerOf(const State<T>& state) noexcept -> ::boyle::math::Vec2<T> {
    return ::boyle::math::Vec2<T>{state.x, state.y};
}

template <std::floating_point T>
[[nodiscard]] auto velocityOf(const State<T>& state) noexcept -> ::boyle::math::Vec2<T> {
    return ::boyle::math::Vec2<T>{state.dx, state.dy};
}

/**
 * @brief Analytic solution under a constant acceleration and a constant steering angle.
 *
 * A constant steering angle gives a constant path curvature that does not depend on the speed, so
 * the center of gravity always traces a circular arc of radius 1 / kappa and the acceleration only
 * changes how fast that arc is traversed.
 */
template <std::floating_point T>
[[nodiscard]] auto analyticAt(const Sample<T>& initial, T accel, T steering_angle, T t) noexcept
    -> Sample<T> {
    const T beta{slipAngleOf(steering_angle)};
    const T kappa{curvatureOf(steering_angle)};
    const T arc_length{initial.speed * t + static_cast<T>(0.5) * accel * t * t};
    const T course{initial.yaw + beta};
    const T speed{initial.speed + accel * t};
    if (kappa == static_cast<T>(0.0)) {
        return Sample<T>{
            {initial.center.x + arc_length * std::cos(course),
             initial.center.y + arc_length * std::sin(course)},
            initial.yaw,
            speed
        };
    }
    return Sample<T>{
        {initial.center.x + (std::sin(course + kappa * arc_length) - std::sin(course)) / kappa,
         initial.center.y - (std::cos(course + kappa * arc_length) - std::cos(course)) / kappa},
        initial.yaw + kappa * arc_length,
        speed
    };
}

/**
 * @brief Error bound the fourth order Runge-Kutta integrator has to stay within.
 *
 * Tight enough that an integrator that silently degraded to forward Euler, whose error is three
 * to five orders of magnitude larger in every scenario below, would fail immediately.
 */
template <std::floating_point T>
[[nodiscard]] constexpr auto rungeKuttaTolerance() noexcept -> T {
    return std::same_as<T, float> ? static_cast<T>(5.0e-3) : static_cast<T>(1.0e-6);
}

template <std::floating_point T>
[[nodiscard]] auto positionErrorOf(const State<T>& state, const Sample<T>& reference) noexcept
    -> T {
    return std::hypot(state.x - reference.center.x, state.y - reference.center.y);
}

template <std::floating_point T>
struct Trajectory final {
    std::vector<T> ts;
    std::vector<State<T>> runge_kutta;
    std::vector<State<T>> forward_euler;
    std::vector<State<T>> symplectic_euler;
    std::vector<Sample<T>> reference;
};

/**
 * @brief Marches every integrator step by step and samples the analytic solution alongside.
 *
 * @param reference_at maps a step index in [1, hs.size()] onto the analytic solution there.
 */
template <std::floating_point T, typename ReferenceFn>
[[nodiscard]] auto simulate(
    const KinematicsModel<T>& model, const State<T>& initial_state, std::span<const T> hs,
    std::span<const T> accels, std::span<const T> steering_angles, const ReferenceFn& reference_at
) -> Trajectory<T> {
    const std::size_t size{hs.size()};
    Trajectory<T> trajectory;
    trajectory.ts.reserve(size + 1);
    trajectory.runge_kutta.reserve(size + 1);
    trajectory.forward_euler.reserve(size + 1);
    trajectory.symplectic_euler.reserve(size + 1);
    trajectory.reference.reserve(size + 1);
    trajectory.ts.push_back(static_cast<T>(0.0));
    trajectory.runge_kutta.push_back(initial_state);
    trajectory.forward_euler.push_back(initial_state);
    trajectory.symplectic_euler.push_back(initial_state);
    trajectory.reference.push_back(reference_at(0));
    for (std::size_t i{0}; i < size; ++i) {
        trajectory.ts.push_back(trajectory.ts.back() + hs[i]);
        trajectory.runge_kutta.push_back(
            model.rungeKutta(trajectory.runge_kutta.back(), hs[i], accels[i], steering_angles[i])
        );
        trajectory.forward_euler.push_back(model.forwardEuler(
            trajectory.forward_euler.back(), hs[i], accels[i], steering_angles[i]
        ));
        trajectory.symplectic_euler.push_back(model.symplecticEuler(
            trajectory.symplectic_euler.back(), hs[i], accels[i], steering_angles[i]
        ));
        trajectory.reference.push_back(reference_at(i + 1));
    }
    return trajectory;
}

/**
 * @brief Asserts that Runge-Kutta tracks the analytic solution and that the batch overloads agree
 *        with marching the single step overloads by hand.
 */
template <std::floating_point T>
auto checkAgainstAnalytic(
    const KinematicsModel<T>& model, const State<T>& initial_state, std::span<const T> hs,
    std::span<const T> accels, std::span<const T> steering_angles, const Trajectory<T>& trajectory,
    T symplectic_euler_bound
) -> void {
    const T tolerance{rungeKuttaTolerance<T>()};
    const std::size_t size{trajectory.ts.size()};
    T max_position_error{static_cast<T>(0.0)};
    T max_yaw_error{static_cast<T>(0.0)};
    T max_speed_error{static_cast<T>(0.0)};
    for (std::size_t i{0}; i < size; ++i) {
        const State<T>& state{trajectory.runge_kutta[i]};
        const Sample<T>& reference{trajectory.reference[i]};
        max_position_error = std::max(max_position_error, positionErrorOf(state, reference));
        max_yaw_error = std::max(max_yaw_error, std::abs(state.yaw - reference.yaw));
        max_speed_error = std::max(max_speed_error, std::abs(state.speed() - reference.speed));
    }
    CHECK_LE(max_position_error, tolerance);
    CHECK_LE(max_yaw_error, tolerance);
    CHECK_LE(max_speed_error, tolerance);

    // symplecticEuler() is only first order, so it gets a loose per scenario bound rather than the
    // Runge-Kutta tolerance. It still has to track the analytic solution: without this an
    // implementation that returned the state untouched would sail through the batch check below.
    T max_symplectic_error{static_cast<T>(0.0)};
    for (std::size_t i{0}; i < size; ++i) {
        max_symplectic_error = std::max(
            max_symplectic_error,
            positionErrorOf(trajectory.symplectic_euler[i], trajectory.reference[i])
        );
    }
    CHECK_LE(max_symplectic_error, symplectic_euler_bound);

    // The batch overloads march the exact same per-step arithmetic as the hand-rolled loop above,
    // so the two are only expected to agree up to the same accuracy the integrator itself is
    // guaranteed to: the compiler is free to inline and vectorize the two call sites differently
    // (e.g. FMA contraction), which shifts the last bits of every step and compounds over hundreds
    // of steps, especially for float under size-optimized builds.
    const State<T> batch_runge_kutta{model.rungeKutta(initial_state, hs, accels, steering_angles)};
    const State<T> batch_forward_euler{
        model.forwardEuler(initial_state, hs, accels, steering_angles)
    };
    const State<T> batch_symplectic_euler{
        model.symplecticEuler(initial_state, hs, accels, steering_angles)
    };
    CHECK_LE(
        positionErrorOf(
            batch_runge_kutta,
            Sample<T>{
                centerOf(trajectory.runge_kutta.back()), trajectory.runge_kutta.back().yaw,
                static_cast<T>(0.0)
            }
        ),
        tolerance
    );
    CHECK_LE(
        positionErrorOf(
            batch_forward_euler,
            Sample<T>{
                centerOf(trajectory.forward_euler.back()), trajectory.forward_euler.back().yaw,
                static_cast<T>(0.0)
            }
        ),
        tolerance
    );
    CHECK_LE(
        positionErrorOf(
            batch_symplectic_euler,
            Sample<T>{
                centerOf(trajectory.symplectic_euler.back()),
                trajectory.symplectic_euler.back().yaw, static_cast<T>(0.0)
            }
        ),
        tolerance
    );
}

auto plotTrajectory(const std::string& title, const Trajectory<double>& trajectory) -> void {
    const std::size_t size{trajectory.ts.size()};
    std::vector<double> reference_xs, reference_ys, runge_kutta_xs, runge_kutta_ys, euler_xs,
        euler_ys, symplectic_xs, symplectic_ys, runge_kutta_speeds, euler_speeds, symplectic_speeds,
        reference_speeds, runge_kutta_yaws, euler_yaws, symplectic_yaws, reference_yaws,
        runge_kutta_errors, euler_errors, symplectic_errors;
    for (std::vector<double>* vec :
         {&reference_xs, &reference_ys, &runge_kutta_xs, &runge_kutta_ys, &euler_xs, &euler_ys,
          &symplectic_xs, &symplectic_ys, &runge_kutta_speeds, &euler_speeds, &symplectic_speeds,
          &reference_speeds, &runge_kutta_yaws, &euler_yaws, &symplectic_yaws, &reference_yaws,
          &runge_kutta_errors, &euler_errors, &symplectic_errors}) {
        vec->reserve(size);
    }
    for (std::size_t i{0}; i < size; ++i) {
        const State<double>& runge_kutta{trajectory.runge_kutta[i]};
        const State<double>& euler{trajectory.forward_euler[i]};
        const State<double>& symplectic{trajectory.symplectic_euler[i]};
        const Sample<double>& reference{trajectory.reference[i]};
        reference_xs.push_back(reference.center.x);
        reference_ys.push_back(reference.center.y);
        reference_speeds.push_back(reference.speed);
        reference_yaws.push_back(reference.yaw);
        runge_kutta_xs.push_back(runge_kutta.x);
        runge_kutta_ys.push_back(runge_kutta.y);
        runge_kutta_speeds.push_back(runge_kutta.speed());
        runge_kutta_yaws.push_back(runge_kutta.yaw);
        runge_kutta_errors.push_back(positionErrorOf(runge_kutta, reference));
        euler_xs.push_back(euler.x);
        euler_ys.push_back(euler.y);
        euler_speeds.push_back(euler.speed());
        euler_yaws.push_back(euler.yaw);
        euler_errors.push_back(positionErrorOf(euler, reference));
        symplectic_xs.push_back(symplectic.x);
        symplectic_ys.push_back(symplectic.y);
        symplectic_speeds.push_back(symplectic.speed());
        symplectic_yaws.push_back(symplectic.yaw);
        symplectic_errors.push_back(positionErrorOf(symplectic, reference));
    }

    matplot::figure_handle fig = createFigureHandle();
    fig->title(title);
    std::vector<matplot::axes_handle> axes = createAxesHandles(fig);
    axes[0]->plot(reference_xs, reference_ys, "-")->line_width(2.0);
    axes[0]->plot(runge_kutta_xs, runge_kutta_ys, "--")->line_width(2.0);
    axes[0]->plot(euler_xs, euler_ys, ":")->line_width(2.0);
    axes[0]->plot(symplectic_xs, symplectic_ys, "-.")->line_width(2.0);
    axes[0]->axis(matplot::equal);
    axes[0]->legend({"analytic", "runge kutta", "forward euler", "symplectic euler"});
    axes[1]->plot(trajectory.ts, reference_speeds, "-")->line_width(2.0);
    axes[1]->plot(trajectory.ts, runge_kutta_speeds, "--")->line_width(2.0);
    axes[1]->plot(trajectory.ts, euler_speeds, ":")->line_width(2.0);
    axes[1]->plot(trajectory.ts, symplectic_speeds, "-.")->line_width(2.0);
    axes[1]->legend({"analytic", "runge kutta", "forward euler", "symplectic euler"});
    axes[2]->plot(trajectory.ts, reference_yaws, "-")->line_width(2.0);
    axes[2]->plot(trajectory.ts, runge_kutta_yaws, "--")->line_width(2.0);
    axes[2]->plot(trajectory.ts, euler_yaws, ":")->line_width(2.0);
    axes[2]->plot(trajectory.ts, symplectic_yaws, "-.")->line_width(2.0);
    axes[2]->legend({"analytic", "runge kutta", "forward euler", "symplectic euler"});
    axes[3]->semilogy(trajectory.ts, runge_kutta_errors, "--")->line_width(2.0);
    axes[3]->semilogy(trajectory.ts, euler_errors, ":")->line_width(2.0);
    axes[3]->semilogy(trajectory.ts, symplectic_errors, "-.")->line_width(2.0);
    axes[3]->legend({"runge kutta", "forward euler", "symplectic euler"});
    fig->show();
}

} // namespace

TEST_CASE_TEMPLATE("StraightLineAccelerateThenDecelerate", T, float, double) {
    const KinematicsModel<T> model{kFrontWheelbase<T>, kRearWheelbase<T>};
    constexpr T kSteeringAngle{static_cast<T>(0.0)};
    constexpr T kInitialSpeed{static_cast<T>(5.0)};
    constexpr T kAccel{static_cast<T>(2.0)};
    constexpr T kPhaseDuration{static_cast<T>(6.0)};
    constexpr std::size_t kStepsPerPhase{1200};
    const T h{kPhaseDuration / static_cast<T>(kStepsPerPhase)};

    const State<T> initial_state{model.createState(
        static_cast<T>(0.0), static_cast<T>(0.0), kInitialYaw<T>, kInitialSpeed, kSteeringAngle
    )};
    const std::vector<T> hs(kStepsPerPhase * 2, h);
    std::vector<T> accels(kStepsPerPhase * 2, kAccel);
    std::fill(accels.begin() + kStepsPerPhase, accels.end(), -kAccel);
    const std::vector<T> steering_angles(kStepsPerPhase * 2, kSteeringAngle);

    const Sample<T> start{centerOf(initial_state), kInitialYaw<T>, kInitialSpeed};
    const Sample<T> midpoint{analyticAt(start, kAccel, kSteeringAngle, kPhaseDuration)};
    const auto reference_at = [&](std::size_t i) -> Sample<T> {
        const T t{h * static_cast<T>(i)};
        return i <= kStepsPerPhase
                   ? analyticAt(start, kAccel, kSteeringAngle, t)
                   : analyticAt(midpoint, -kAccel, kSteeringAngle, t - kPhaseDuration);
    };

    const Trajectory<T> trajectory{
        simulate<T>(model, initial_state, hs, accels, steering_angles, reference_at)
    };
    // symplecticEuler() is first order and lands about 0.03 m off over this run, so the
    // bound leaves roughly a factor of two of room while still failing outright on an
    // implementation that does not integrate.
    constexpr T kSymplecticEulerBound{static_cast<T>(0.06)};
    checkAgainstAnalytic<T>(
        model, initial_state, hs, accels, steering_angles, trajectory, kSymplecticEulerBound
    );

    // A zero steering angle has to leave the heading and the yaw rate untouched throughout.
    for (const State<T>& state : trajectory.runge_kutta) {
        CHECK_EQ(state.yaw, doctest::Approx(kInitialYaw<T>).epsilon(rungeKuttaTolerance<T>()));
        CHECK_LE(std::abs(state.dyaw), rungeKuttaTolerance<T>());
    }
    // The speed comes back to where it started after the symmetric acceleration profile.
    CHECK_EQ(
        velocityOf(trajectory.runge_kutta.back()).euclidean(),
        doctest::Approx(kInitialSpeed).epsilon(rungeKuttaTolerance<T>())
    );

    if (plot_graph) {
        if constexpr (std::same_as<T, double>) {
            plotTrajectory("straight line accelerate then decelerate", trajectory);
        }
    }
}

TEST_CASE_TEMPLATE("ConstantSpeedCircle", T, float, double) {
    const KinematicsModel<T> model{kFrontWheelbase<T>, kRearWheelbase<T>};
    constexpr T kSteeringAngle{static_cast<T>(0.15)};
    constexpr T kInitialSpeed{static_cast<T>(8.0)};
    constexpr T kAccel{static_cast<T>(0.0)};
    constexpr std::size_t kNumSteps{800};

    const T kappa{curvatureOf(kSteeringAngle)};
    const T radius{static_cast<T>(1.0) / kappa};
    // A constant steering angle puts the center of gravity on a circle whose radius follows from
    // the geometry alone: R = hypot(l_r, wheelbase / tan(delta)).
    CHECK_EQ(
        radius, doctest::Approx(
                    std::hypot(
                        kRearWheelbase<T>,
                        (kFrontWheelbase<T> + kRearWheelbase<T>) / std::tan(kSteeringAngle)
                    )
                )
                    .epsilon(rungeKuttaTolerance<T>())
    );

    const T period{static_cast<T>(2.0) * std::numbers::pi_v<T> / (kappa * kInitialSpeed)};
    const T h{period / static_cast<T>(kNumSteps)};
    const State<T> initial_state{model.createState(
        static_cast<T>(0.0), static_cast<T>(0.0), kInitialYaw<T>, kInitialSpeed, kSteeringAngle
    )};
    const std::vector<T> hs(kNumSteps, h);
    const std::vector<T> accels(kNumSteps, kAccel);
    const std::vector<T> steering_angles(kNumSteps, kSteeringAngle);

    const Sample<T> start{centerOf(initial_state), kInitialYaw<T>, kInitialSpeed};
    const auto reference_at = [&](std::size_t i) -> Sample<T> {
        return analyticAt(start, kAccel, kSteeringAngle, h * static_cast<T>(i));
    };

    const Trajectory<T> trajectory{
        simulate<T>(model, initial_state, hs, accels, steering_angles, reference_at)
    };
    // symplecticEuler() is first order and lands about 0.48 m off over this run, so the
    // bound leaves roughly a factor of two of room while still failing outright on an
    // implementation that does not integrate.
    constexpr T kSymplecticEulerBound{static_cast<T>(1.0)};
    checkAgainstAnalytic<T>(
        model, initial_state, hs, accels, steering_angles, trajectory, kSymplecticEulerBound
    );

    // Every sample has to sit on the circle centered at the instantaneous center of rotation.
    const T course{kInitialYaw<T> + slipAngleOf(kSteeringAngle)};
    const ::boyle::math::Vec2<T> rotation_center{
        start.center.x - radius * std::sin(course), start.center.y + radius * std::cos(course)
    };
    for (const State<T>& state : trajectory.runge_kutta) {
        CHECK_EQ(
            centerOf(state).euclideanTo(rotation_center),
            doctest::Approx(radius).epsilon(rungeKuttaTolerance<T>())
        );
        CHECK_EQ(
            velocityOf(state).euclidean(),
            doctest::Approx(kInitialSpeed).epsilon(rungeKuttaTolerance<T>())
        );
        CHECK_EQ(
            state.dyaw, doctest::Approx(kappa * kInitialSpeed).epsilon(rungeKuttaTolerance<T>())
        );
    }
    // One full period brings the vehicle back to where it started.
    CHECK_LE(
        centerOf(trajectory.runge_kutta.back()).euclideanTo(start.center), rungeKuttaTolerance<T>()
    );
    CHECK_EQ(
        trajectory.runge_kutta.back().yaw,
        doctest::Approx(kInitialYaw<T> + static_cast<T>(2.0) * std::numbers::pi_v<T>)
            .epsilon(rungeKuttaTolerance<T>())
    );

    if (plot_graph) {
        if constexpr (std::same_as<T, double>) {
            plotTrajectory("constant speed circle", trajectory);
        }
    }
}

TEST_CASE_TEMPLATE("AcceleratingTurn", T, float, double) {
    const KinematicsModel<T> model{kFrontWheelbase<T>, kRearWheelbase<T>};
    constexpr T kSteeringAngle{static_cast<T>(0.15)};
    constexpr T kInitialSpeed{static_cast<T>(8.0)};
    constexpr T kAccel{static_cast<T>(1.5)};
    constexpr T kDuration{static_cast<T>(12.0)};
    constexpr std::size_t kNumSteps{2400};
    const T h{kDuration / static_cast<T>(kNumSteps)};

    const State<T> initial_state{model.createState(
        static_cast<T>(0.0), static_cast<T>(0.0), kInitialYaw<T>, kInitialSpeed, kSteeringAngle
    )};
    const std::vector<T> hs(kNumSteps, h);
    const std::vector<T> accels(kNumSteps, kAccel);
    const std::vector<T> steering_angles(kNumSteps, kSteeringAngle);

    const Sample<T> start{centerOf(initial_state), kInitialYaw<T>, kInitialSpeed};
    const auto reference_at = [&](std::size_t i) -> Sample<T> {
        return analyticAt(start, kAccel, kSteeringAngle, h * static_cast<T>(i));
    };

    const Trajectory<T> trajectory{
        simulate<T>(model, initial_state, hs, accels, steering_angles, reference_at)
    };
    // symplecticEuler() is first order and lands about 0.29 m off over this run, so the
    // bound leaves roughly a factor of two of room while still failing outright on an
    // implementation that does not integrate.
    constexpr T kSymplecticEulerBound{static_cast<T>(0.6)};
    checkAgainstAnalytic<T>(
        model, initial_state, hs, accels, steering_angles, trajectory, kSymplecticEulerBound
    );

    // Accelerating through a turn is where the acceleration has to be resolved along the course
    // angle yaw + beta rather than along the heading: doing it along the heading alone leaves the
    // speed growing as accel * cos(beta) and pulls the velocity off the course angle, an error
    // that neither the straight line nor the constant speed circle scenario can expose.
    const T kappa{curvatureOf(kSteeringAngle)};
    for (std::size_t i{0}; i < trajectory.ts.size(); ++i) {
        const State<T>& state{trajectory.runge_kutta[i]};
        const T t{trajectory.ts[i]};
        const T speed{kInitialSpeed + kAccel * t};
        CHECK_EQ(
            velocityOf(state).euclidean(), doctest::Approx(speed).epsilon(rungeKuttaTolerance<T>())
        );
        // state.yaw accumulates without wrapping (see ConstantSpeedCircle, which drives it past a
        // full turn on purpose), while atan2() always answers in (-pi, pi], so the two only agree
        // modulo 2 * pi once the course angle winds past that range over the run.
        const T course_angle{state.yaw + slipAngleOf(kSteeringAngle)};
        const T velocity_angle{std::atan2(state.dy, state.dx)};
        CHECK_LE(
            std::abs(
                std::remainder(
                    velocity_angle - course_angle, static_cast<T>(2.0) * std::numbers::pi_v<T>
                )
            ),
            rungeKuttaTolerance<T>()
        );
        CHECK_EQ(state.dyaw, doctest::Approx(kappa * speed).epsilon(rungeKuttaTolerance<T>()));
    }

    if (plot_graph) {
        if constexpr (std::same_as<T, double>) {
            plotTrajectory("accelerating turn", trajectory);
        }
    }
}

TEST_CASE_TEMPLATE("DecelerateThroughZeroIntoReverse", T, float, double) {
    const KinematicsModel<T> model{kFrontWheelbase<T>, kRearWheelbase<T>};
    constexpr T kSteeringAngle{static_cast<T>(0.15)};
    constexpr T kInitialSpeed{static_cast<T>(12.0)};
    constexpr T kAccel{static_cast<T>(-2.0)};
    constexpr T kDuration{static_cast<T>(12.0)};
    constexpr std::size_t kNumSteps{2400};
    const T h{kDuration / static_cast<T>(kNumSteps)};

    const State<T> initial_state{model.createState(
        static_cast<T>(0.0), static_cast<T>(0.0), kInitialYaw<T>, kInitialSpeed, kSteeringAngle
    )};
    const std::vector<T> hs(kNumSteps, h);
    const std::vector<T> accels(kNumSteps, kAccel);
    const std::vector<T> steering_angles(kNumSteps, kSteeringAngle);

    const Sample<T> start{centerOf(initial_state), kInitialYaw<T>, kInitialSpeed};
    const auto reference_at = [&](std::size_t i) -> Sample<T> {
        return analyticAt(start, kAccel, kSteeringAngle, h * static_cast<T>(i));
    };

    const Trajectory<T> trajectory{
        simulate<T>(model, initial_state, hs, accels, steering_angles, reference_at)
    };
    // symplecticEuler() is first order and lands about 0.66 m off over this run, so the
    // bound leaves roughly a factor of two of room while still failing outright on an
    // implementation that does not integrate.
    constexpr T kSymplecticEulerBound{static_cast<T>(1.4)};
    checkAgainstAnalytic<T>(
        model, initial_state, hs, accels, steering_angles, trajectory, kSymplecticEulerBound
    );

    // The speed runs from +12 down through zero to -12, so the run crosses the one point where the
    // velocity vector carries no direction at all. Recovering the sign from the projection onto
    // the heading is what lets the trajectory continue into reverse instead of bouncing back:
    // taking the magnitude alone leaves the vehicle turning the wrong way once it reverses.
    const T beta{slipAngleOf(kSteeringAngle)};
    const T kappa{curvatureOf(kSteeringAngle)};
    bool saw_forward{false};
    bool saw_reverse{false};
    for (std::size_t i{0}; i < trajectory.ts.size(); ++i) {
        const State<T>& state{trajectory.runge_kutta[i]};
        const T speed{kInitialSpeed + kAccel * trajectory.ts[i]};
        CHECK_EQ(state.speed(), doctest::Approx(speed).epsilon(rungeKuttaTolerance<T>()));
        CHECK_EQ(state.dyaw, doctest::Approx(kappa * speed).epsilon(rungeKuttaTolerance<T>()));
        // The magnitude stays non-negative throughout and therefore cannot tell the two halves
        // of the run apart on its own.
        CHECK_EQ(
            velocityOf(state).euclidean(),
            doctest::Approx(std::abs(speed)).epsilon(rungeKuttaTolerance<T>())
        );
        // state.yaw accumulates without wrapping, while atan2() always answers in (-pi, pi], so
        // the two only agree modulo 2 * pi once the course angle winds past that range over the
        // run (see the matching comment in AcceleratingTurn).
        const T course_angle{state.yaw + beta};
        if (speed > static_cast<T>(1.0)) {
            saw_forward = true;
            const T velocity_angle{std::atan2(state.dy, state.dx)};
            CHECK_LE(
                std::abs(
                    std::remainder(
                        velocity_angle - course_angle, static_cast<T>(2.0) * std::numbers::pi_v<T>
                    )
                ),
                rungeKuttaTolerance<T>()
            );
        } else if (speed < static_cast<T>(-1.0)) {
            saw_reverse = true;
            // Reversing points the velocity the other way round while the heading is unchanged.
            const T reverse_velocity_angle{std::atan2(-state.dy, -state.dx)};
            CHECK_LE(
                std::abs(
                    std::remainder(
                        reverse_velocity_angle - course_angle,
                        static_cast<T>(2.0) * std::numbers::pi_v<T>
                    )
                ),
                rungeKuttaTolerance<T>()
            );
            // A left steering angle turns the vehicle clockwise once it drives backwards.
            CHECK_LT(state.dyaw, static_cast<T>(0.0));
        } else {
            // Around the reversal the velocity vector shrinks to zero and carries no direction
            // worth asserting on, so only the signed speed above is checked in that band.
        }
    }
    CHECK(saw_forward);
    CHECK(saw_reverse);
    CHECK_LT(trajectory.runge_kutta.back().speed(), static_cast<T>(0.0));

    if (plot_graph) {
        if constexpr (std::same_as<T, double>) {
            plotTrajectory("decelerate through zero into reverse", trajectory);
        }
    }
}

TEST_CASE_TEMPLATE("ModelSerializationRoundtrip", T, float, double) {
    const KinematicsModel<T> model{kFrontWheelbase<T>, kRearWheelbase<T>};
    auto [data, in, out] = zpp::bits::data_in_out();
    REQUIRE(out(model).code == std::errc{});
    KinematicsModel<T> restored{};
    REQUIRE(in(restored).code == std::errc{});
    CHECK_EQ(restored.front_wheelbase(), model.front_wheelbase());
    CHECK_EQ(restored.rear_wheelbase(), model.rear_wheelbase());
    CHECK_EQ(restored.wheelbase(), model.wheelbase());
    // The geometry no longer travels inside a State, so a restored model has to reproduce the
    // states of the original bit for bit.
    CHECK_EQ(
        restored.createState(
            static_cast<T>(0.0), static_cast<T>(0.0), kInitialYaw<T>, static_cast<T>(8.0),
            static_cast<T>(0.15)
        ),
        model.createState(
            static_cast<T>(0.0), static_cast<T>(0.0), kInitialYaw<T>, static_cast<T>(8.0),
            static_cast<T>(0.15)
        )
    );
}

#if BOYLE_CHECK_PARAMS == 1

TEST_CASE_TEMPLATE("RejectsInvalidArguments", T, float, double) {
    const KinematicsModel<T> model{kFrontWheelbase<T>, kRearWheelbase<T>};
    constexpr T kSteeringAngle{static_cast<T>(0.15)};
    constexpr T kOriginX{static_cast<T>(0.0)};
    constexpr T kOriginY{static_cast<T>(0.0)};

    SUBCASE("ModelRejectsNonPositiveWheelbase") {
        CHECK_THROWS_AS(
            (KinematicsModel<T>{static_cast<T>(0.0), kRearWheelbase<T>}), std::invalid_argument
        );
        CHECK_THROWS_AS(
            (KinematicsModel<T>{kFrontWheelbase<T>, static_cast<T>(-1.0)}), std::invalid_argument
        );
    }

    SUBCASE("CreateStateRejectsSingularSteeringAngle") {
        CHECK_THROWS_AS(
            static_cast<void>(model.createState(
                kOriginX, kOriginY, kInitialYaw<T>, static_cast<T>(8.0),
                std::numbers::pi_v<T> * static_cast<T>(0.5)
            )),
            std::invalid_argument
        );
    }

    SUBCASE("IntegratorsRejectNonPositiveTimeStep") {
        const State<T> state{model.createState(
            kOriginX, kOriginY, kInitialYaw<T>, static_cast<T>(8.0), kSteeringAngle
        )};
        CHECK_THROWS_AS(
            static_cast<void>(
                model.forwardEuler(state, static_cast<T>(0.0), static_cast<T>(1.0), kSteeringAngle)
            ),
            std::invalid_argument
        );
        CHECK_THROWS_AS(
            static_cast<void>(
                model.rungeKutta(state, static_cast<T>(-0.01), static_cast<T>(1.0), kSteeringAngle)
            ),
            std::invalid_argument
        );
    }

    SUBCASE("IntegratorsRejectSingularSteeringAngle") {
        const State<T> state{model.createState(
            kOriginX, kOriginY, kInitialYaw<T>, static_cast<T>(8.0), kSteeringAngle
        )};
        CHECK_THROWS_AS(
            static_cast<void>(model.rungeKutta(
                state, static_cast<T>(0.01), static_cast<T>(1.0),
                std::numbers::pi_v<T> * static_cast<T>(0.5)
            )),
            std::invalid_argument
        );
    }

    SUBCASE("BatchOverloadsRejectMismatchedSpanSizes") {
        const State<T> state{model.createState(
            kOriginX, kOriginY, kInitialYaw<T>, static_cast<T>(8.0), kSteeringAngle
        )};
        const std::vector<T> hs(4, static_cast<T>(0.01));
        const std::vector<T> accels(3, static_cast<T>(1.0));
        const std::vector<T> steering_angles(4, kSteeringAngle);
        CHECK_THROWS_AS(
            static_cast<void>(model.forwardEuler(
                state, std::span<const T>{hs}, std::span<const T>{accels},
                std::span<const T>{steering_angles}
            )),
            std::invalid_argument
        );
        CHECK_THROWS_AS(
            static_cast<void>(model.rungeKutta(
                state, std::span<const T>{hs}, std::span<const T>{accels},
                std::span<const T>{steering_angles}
            )),
            std::invalid_argument
        );
    }
}

#endif

} // namespace boyle::bicycle

auto main(int argc, const char* argv[]) -> int {
    cxxopts::Options options("kinematics_model_test", "unit test of KinematicsModel class");
    options.add_options()(
        "plot-graph", "plot test graph", cxxopts::value<bool>()->default_value("false")
    );
    cxxopts::ParseResult result = options.parse(argc, argv);
    plot_graph = result["plot-graph"].as<bool>();
    doctest::Context context(argc, argv);
    return context.run();
}
