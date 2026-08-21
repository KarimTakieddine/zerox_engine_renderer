#pragma once

#include <cstddef>
#include <span>
#include <type_traits>

namespace renderer
{
    template<typename T, bool IsConst = false, size_t L = 1>
    using FixedSpan = std::span<std::conditional_t<IsConst, const T, T>, L>;

    template<typename T, bool IsConst = false>
    using DynamicSpan = std::span<std::conditional_t<IsConst, const T, T>>;
}
