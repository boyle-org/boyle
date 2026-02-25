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

#include <concepts>

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
};

} // namespace boyle::bicycle
