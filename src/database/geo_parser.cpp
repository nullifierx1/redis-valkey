#include <database/geo_parser.hpp>
#include <types/db_value.hpp>
#include <types/result.hpp>

#include <span>
#include <variant>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <optional>

namespace gp_helpers {

std::string ToLower(std::string value) {
    for (char& ch : value) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return value;
}

bool EqualsToken(const std::string& value, std::string_view expected) {
    if (value.size() != expected.size()) return false;

    for (size_t i = 0; i < value.size(); ++i) {
        unsigned char ch = static_cast<unsigned char>(value[i]);
        if (std::toupper(ch) != expected[i]) return false;
    }

    return true;
}

std::optional<GeoValue::coordinates_type> ParseCoordinate(const std::string& value) {
    try {
        size_t parsed_size = 0;
        double coordinate = std::stod(value, &parsed_size);

        if (parsed_size != value.size() || !std::isfinite(coordinate)) {
            return std::nullopt;
        }

        return static_cast<GeoValue::coordinates_type>(coordinate);
    } catch (const std::invalid_argument&) {
        return std::nullopt;
    } catch (const std::out_of_range&) {
        return std::nullopt;
    }
}

std::optional<int> ParsePositiveCount(const std::string& text) {
    if (text.empty()) return std::nullopt;

    int value = 0;
    for (char ch : text) {
        if (!std::isdigit(static_cast<unsigned char>(ch))) {
            return std::nullopt;
        }
        value = value * 10 + static_cast<int>(ch - '0');
    }

    if (value <= 0) return std::nullopt;
    return value;
}

bool IsValidLongitude(GeoValue::coordinates_type longitude) {
    return longitude >= -180.0L && longitude <= 180.0L;
}

bool IsValidLatitude(GeoValue::coordinates_type latitude) {
    return latitude >= -85.05112878L && latitude <= 85.05112878L;
}

std::optional<double> ParseUnitMultiplier(const std::string& unit) {
    std::string lower_unit = ToLower(unit);

    if (lower_unit == "m") return 1000.0;
    if (lower_unit == "km") return 1.0;
    if (lower_unit == "mi") return 0.621371192;
    if (lower_unit == "ft") return 3280.839895;

    return std::nullopt;
}

bool SetSort(std::optional<bool>& sort_ascending, bool value) {
    if (sort_ascending.has_value()) return false;

    sort_ascending = value;
    return true;
}

std::optional<Result> ParseSortAndCount(
    std::span<const std::string> args,
    size_t& pos,
    std::optional<bool>& sort_ascending,
    std::optional<int>& count
) {
    while (pos < args.size()) {
        if (EqualsToken(args[pos], "ASC")) {
            if (!SetSort(sort_ascending, true)) {
                return Result::Error("syntax error");
            }
            ++pos;

        } else if (EqualsToken(args[pos], "DESC")) {
            if (!SetSort(sort_ascending, false)) {
                return Result::Error("syntax error");
            }
            ++pos;

        } else if (EqualsToken(args[pos], "COUNT")) {
            if (count.has_value() || pos + 1 >= args.size()) {
                return Result::Error("syntax error");
            }

            count = ParsePositiveCount(args[pos + 1]);
            if (!count) return Result::Error("invalid count");
            pos += 2;

        } else {
            return Result::Error("syntax error");
        }
    }

    return std::nullopt;
}

std::optional<Result> ParseSearchPrefix(
    std::span<const std::string> args,
    GeoValue::Location& center,
    double& radius,
    double& unit_multiplier
) {
    if (args.size() < 6) {
        return Result::Error("wrong number of arguments");
    }

    if (!EqualsToken(args[0], "FROMLONLAT") ||
        !EqualsToken(args[3], "BYRADIUS")) {
        return Result::Error("syntax error");
    }

    auto parsed_center = GeoParser::ParseLocation(args[1], args[2]);
    if (const auto* error = std::get_if<Result>(&parsed_center)) {
        return *error;
    }

    auto parsed_radius = ParseCoordinate(args[4]);
    if (!parsed_radius || *parsed_radius < 0.0L) return Result::Error("invalid radius");

    auto parsed_unit = GeoParser::ParseUnit(args[5]);
    if (const auto* error = std::get_if<Result>(&parsed_unit)) {
        return *error;
    }

    center = std::get<GeoValue::Location>(parsed_center);
    radius = static_cast<double>(*parsed_radius);
    unit_multiplier = std::get<double>(parsed_unit);
    return std::nullopt;
}

} // namespace gp_helpers

using namespace gp_helpers;

GeoParser::UnitParseResult GeoParser::ParseUnit(const std::string& unit) {
    auto multiplier = gp_helpers::ParseUnitMultiplier(unit);
    if (!multiplier) return Result::Error("unsupported unit");
    return *multiplier;
}

GeoParser::LocationParseResult GeoParser::ParseLocation(
    const std::string& longitude_text,
    const std::string& latitude_text
) {
    auto longitude = gp_helpers::ParseCoordinate(longitude_text);
    auto latitude = gp_helpers::ParseCoordinate(latitude_text);

    if (!longitude || !latitude ||
        !gp_helpers::IsValidLongitude(*longitude) 
        || !gp_helpers::IsValidLatitude(*latitude)) {
        return Result::Error("invalid longitude or latitude");
    }

    GeoValue::Location location;
    location.longitude = *longitude;
    location.latitude = *latitude;
    return location;
}

GeoParser::SearchParseResult GeoParser::ParseSearch(std::span<const std::string> args) {
    GeoValue::Location center;
    double radius = 0.0;
    double unit_multiplier = 1000.0;

    auto prefix_error = ParseSearchPrefix(
        args, center,
        radius, unit_multiplier
    );
    if (prefix_error) return *prefix_error;

    std::optional<bool> sort_ascending;
    std::optional<int> count;

    size_t pos = 6;
    auto options_error = ParseSortAndCount(
        args, pos,
        sort_ascending, count
    );
    if (options_error) return *options_error;

    SearchArgs result;
    result.center = center;
    result.radius = radius;
    result.unit_multiplier = unit_multiplier;
    result.sort_asc = sort_ascending;
    result.count = count;
    return result;
}