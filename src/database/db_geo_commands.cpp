#include <database/database.hpp>
#include <database/geo_parser.hpp>
#include <types/db_value.hpp>
#include <types/result.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <utility>
#include <variant>
#include <vector>
#include <ios>

constexpr double kEarthRadiusKm = 6372.8;

namespace geo_helpers {

struct GeoSearchMatch {
    std::string member;
    double distance = 0.0;
};

std::string FormatCoordinate(GeoValue::coordinates_type coordinate) {
    std::ostringstream output;
    output << std::fixed << std::setprecision(2) << static_cast<double>(coordinate);
    return output.str();
}

double ToRadians(GeoValue::coordinates_type degrees) {
    static const double kPI = std::acos(-1.0);
    return static_cast<double>(degrees) * kPI / 180.0;
}

double HaversineDistanceKm(
    const GeoValue::Location& first,
    const GeoValue::Location& second
) {
    double lat1 = ToRadians(first.latitude);
    double lat2 = ToRadians(second.latitude);
    double delta_latitude = ToRadians(second.latitude - first.latitude);
    double delta_longitude = ToRadians(second.longitude - first.longitude);

    double sin_latitude = std::sin(delta_latitude / 2.0);
    double sin_longitude = std::sin(delta_longitude / 2.0);

    double a = sin_latitude * sin_latitude +
        std::cos(lat1) * std::cos(lat2) * sin_longitude * sin_longitude;
    double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));

    return kEarthRadiusKm * c;
}

std::string FormatDistance(double distance) {
    std::ostringstream output;
    output << std::fixed << std::setprecision(2) << distance;
    return output.str();
}

void SortMatches(
    std::vector<GeoSearchMatch>& matches,
    bool sort_ascending
) {
    std::sort(matches.begin(), matches.end(),
        [sort_ascending](
            const GeoSearchMatch& left,
            const GeoSearchMatch& right
        ) {
            return sort_ascending
                ? left.distance < right.distance
                : left.distance > right.distance;
        }
    );
}

void ApplySearchOptions(
    std::vector<GeoSearchMatch>& matches,
    const GeoParser::SearchArgs& search
) {
    if (search.sort_asc.has_value()) {
        SortMatches(matches, *search.sort_asc);
    }

    if (search.count.has_value() &&
        static_cast<size_t>(*search.count) < matches.size()) {
        matches.resize(static_cast<size_t>(*search.count));
    }
}

std::vector<std::string> MemberNames(const std::vector<GeoSearchMatch>& matches) {
    std::vector<std::string> result;
    result.reserve(matches.size());

    for (const auto& match : matches) {
        result.push_back(match.member);
    }

    return result;
}

std::vector<GeoSearchMatch> FindRawMatches(
    const GeoValue& geo,
    const GeoValue::Location& center,
    double radius,
    double unit_multiplier
) {
    std::vector<GeoSearchMatch> matches;

    for (const auto& [member, location] : geo.data) {
        double distance = HaversineDistanceKm(center, location) * unit_multiplier;
        if (distance <= radius) {
            matches.push_back(GeoSearchMatch{member, distance});
        }
    }

    return matches;
}

std::vector<GeoSearchMatch> FindSearchMatches(
    const GeoValue& geo,
    const GeoParser::SearchArgs& search
) {
    auto matches = FindRawMatches(
        geo,
        search.center,
        search.radius,
        search.unit_multiplier
    );
    ApplySearchOptions(matches, search);
    return matches;
}

GeoValue MakeStoredGeoValue(
    const GeoValue& source_geo,
    const std::vector<GeoSearchMatch>& matches
) {
    GeoValue stored_geo;

    for (const auto& match : matches) {
        auto found = source_geo.data.find(match.member);
        if (found != source_geo.data.end()) {
            stored_geo.data.insert_or_assign(match.member, found->second);
        }
    }

    return stored_geo;
}

} // namespace geo_helpers

Result DataBase::Geoadd(
    const std::string& key,
    std::span<const std::string> locations
) {
    std::vector<GeoValue::Location> parsed_locations;
    parsed_locations.reserve(locations.size() / 3);

    for (size_t i = 0; i < locations.size(); i += 3) {
        auto location = GeoParser::ParseLocation(locations[i], locations[i + 1]);
        if (const auto* error = std::get_if<Result>(&location)) {
            return *error;
        }

        auto parsed_location = std::get<GeoValue::Location>(location);
        parsed_location.member = locations[i + 2];
        parsed_locations.push_back(std::move(parsed_location));
    }

    RedisItem* item = FindItem(key);
    if (item != nullptr && std::get_if<GeoValue>(&item->value) == nullptr) {
        return WrongTypeError();
    }

    if (PossibleToAdd(ValuesMemoryUsage(locations))) return OomError();

    GeoValue* geo = GetOrCreateValue<GeoValue>(key);
    int64_t added_count = 0;
    for (auto& parsed_location : parsed_locations) {
        const std::string& member = parsed_location.member;
        if (!geo->data.contains(member)) ++added_count;

        geo->data.insert_or_assign(member, std::move(parsed_location));
    }

    return Result::Integer(added_count);
}

Result DataBase::Geopos(
    const std::string& key,
    std::span<const std::string> locations
) const {
    const auto* item = FindItem(key);
    std::vector<std::optional<GeoPosition>> result;
    result.reserve(locations.size());

    if (item == nullptr) {
        result.resize(locations.size());
        return Result::GeoPositions(std::move(result));
    }

    const auto* geo = std::get_if<GeoValue>(&item->value);
    if (geo == nullptr) return WrongTypeError();

    for (const std::string& member : locations) {
        auto found = geo->data.find(member);
        if (found == geo->data.end()) {
            result.push_back(std::nullopt);
            continue;
        }

        const auto& position = found->second;
        GeoPosition geo_position;
        geo_position.longitude = geo_helpers::FormatCoordinate(position.longitude);
        geo_position.latitude = geo_helpers::FormatCoordinate(position.latitude);

        result.push_back(std::move(geo_position));
    }

    return Result::GeoPositions(std::move(result));
}

Result DataBase::Geodist(
    const std::string& key,
    const std::string& member1,
    const std::string& member2,
    const std::string& unit
) const {
    auto parsed_unit = GeoParser::ParseUnit(unit);
    if (const auto* error = std::get_if<Result>(&parsed_unit)) {
        return *error;
    }

    const auto* item = FindItem(key);
    if (item == nullptr) return Result::Nil();

    const auto* geo = std::get_if<GeoValue>(&item->value);
    if (geo == nullptr) return WrongTypeError();

    auto first = geo->data.find(member1);
    auto second = geo->data.find(member2);
    if (first == geo->data.end() || second == geo->data.end()) {
        return Result::Nil();
    }

    double distance = geo_helpers::HaversineDistanceKm(
        first->second,
        second->second
    ) * std::get<double>(parsed_unit);

    return Result::String(geo_helpers::FormatDistance(distance));
}

Result DataBase::Geosearch(
    const std::string& key,
    std::span<const std::string> args
) const {
    auto parsed = GeoParser::ParseSearch(args);
    if (const auto* error = std::get_if<Result>(&parsed)) {
        return *error;
    }

    const auto& search = std::get<GeoParser::SearchArgs>(parsed);

    const auto* item = FindItem(key);
    if (item == nullptr) return Result::Array({});

    const auto* geo = std::get_if<GeoValue>(&item->value);
    if (geo == nullptr) return WrongTypeError();

    auto matches = geo_helpers::FindSearchMatches(*geo, search);
    return Result::Array(geo_helpers::MemberNames(matches));
}

Result DataBase::Geosearchstore(
    const std::string& dest,
    const std::string& source,
    std::span<const std::string> args
) {
    auto parsed = GeoParser::ParseSearch(args);
    if (const auto* error = std::get_if<Result>(&parsed)) {
        return *error;
    }

    const auto& search = std::get<GeoParser::SearchArgs>(parsed);
    const auto* item = FindItem(source);
    if (item == nullptr) return Result::Integer(0);

    const auto* geo = std::get_if<GeoValue>(&item->value);
    if (geo == nullptr) return WrongTypeError();

    auto matches = geo_helpers::FindSearchMatches(*geo, search);
    GeoValue stored_geo = geo_helpers::MakeStoredGeoValue(*geo, matches);
    RedisItem stored_item(Value{stored_geo});

    RedisItem* dest_item = FindItem(dest);
    if (dest_item != nullptr && std::get_if<GeoValue>(&dest_item->value) == nullptr) {
        return WrongTypeError();
    }

    if (PossibleToAdd(ItemMemoryUsage(dest, stored_item))) return OomError();

    auto* dest_geo = GetOrCreateValue<GeoValue>(dest);
    *dest_geo = std::move(stored_geo);

    return Result::Integer(static_cast<int64_t>(matches.size()));
}