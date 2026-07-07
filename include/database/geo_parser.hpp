#pragma once

#include <types/db_value.hpp>
#include <types/result.hpp>

#include <optional>
#include <span>
#include <string>
#include <variant>

class GeoParser final {
public:
    struct SearchArgs {
        GeoValue::Location center;
        double radius = 0.0;
        double unit_multiplier = 1000.0;
        std::optional<bool> sort_asc;
        std::optional<int> count;
    };

    using LocationParseResult = std::variant<GeoValue::Location, Result>;
    using UnitParseResult = std::variant<double, Result>;
    using SearchParseResult = std::variant<SearchArgs, Result>;

    GeoParser() = delete;

    static UnitParseResult ParseUnit(const std::string& unit);
    static LocationParseResult ParseLocation(
        const std::string& longitude_text,
        const std::string& latitude_text
    );
    static SearchParseResult ParseSearch(std::span<const std::string> args);
};