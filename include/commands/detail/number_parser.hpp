#pragma once

#include <concepts/decimal_integer.hpp>

#include <charconv>
#include <concepts>
#include <optional>
#include <string_view>
#include <system_error>

class NumberParser final {
public:
    NumberParser() = delete;

    template <DecimalInteger Integer>
    [[nodiscard]] static std::optional<Integer> ParseNonNegativeDecimal(std::string_view text) {
        for (char ch : text) {
            if (ch < '0' || ch > '9') return std::nullopt;
        }
        return ParseDecimal<Integer>(text);
    }

    template <DecimalInteger Integer>
        requires std::signed_integral<Integer>
    [[nodiscard]] static std::optional<Integer> ParseSignedDecimal(std::string_view text) {
        return ParseDecimal<Integer>(text);
    }

private:
    template <DecimalInteger Integer>
    [[nodiscard]] static std::optional<Integer> ParseDecimal(std::string_view text) {
        if (text.empty()) return std::nullopt;

        Integer value{};
        const auto [end, error] = std::from_chars(
            text.data(), text.data() + text.size(), value
        );
        if (error != std::errc{} || end != text.data() + text.size()) {
            return std::nullopt;
        }
        return value;
    }
};
