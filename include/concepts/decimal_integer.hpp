#pragma once

#include <type_traits>

template <typename T>
concept DecimalInteger =
    std::is_integral_v<T> &&
    std::is_same_v<T, std::remove_cv_t<T>> &&
    !std::disjunction_v<
        std::is_same<T, bool>,
        std::is_same<T, wchar_t>,
        std::is_same<T, char8_t>,
        std::is_same<T, char16_t>,
        std::is_same<T, char32_t>
    >;
