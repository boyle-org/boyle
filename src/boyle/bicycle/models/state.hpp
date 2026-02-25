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

#include <concepts>

namespace boyle::bicycle {

template <std::floating_point T>
struct State final {
    using value_type = T;
};

} // namespace boyle::bicycle
