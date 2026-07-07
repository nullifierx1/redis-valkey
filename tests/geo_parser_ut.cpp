#include "result_test_utils.hpp"

#include <database/geo_parser.hpp>
#include <types/db_value.hpp>
#include <types/result.hpp>

#include <gtest/gtest.h>

#include <variant>
#include <string>
#include <optional>

namespace {

GeoParser::SearchArgs ExpectSearchArgs(const GeoParser::SearchParseResult& result) {
    const auto* args = std::get_if<GeoParser::SearchArgs>(&result);
    EXPECT_NE(args, nullptr);
    return args == nullptr ? GeoParser::SearchArgs{} : *args;
}

GeoValue::Location ExpectLocation(const GeoParser::LocationParseResult& result) {
    const auto* location = std::get_if<GeoValue::Location>(&result);
    EXPECT_NE(location, nullptr);
    return location == nullptr ? GeoValue::Location{} : *location;
}

double ExpectUnit(const GeoParser::UnitParseResult& result) {
    const auto* unit = std::get_if<double>(&result);
    EXPECT_NE(unit, nullptr);
    return unit == nullptr ? 0.0 : *unit;
}

void ExpectParseError(const Result& result, const std::string& message) {
    ExpectError(result, message);
}

} // namespace

TEST(GeoParser, ParsesValidLocation) {
    const auto location = ExpectLocation(GeoParser::ParseLocation("13.5", "38.25"));

    EXPECT_DOUBLE_EQ(static_cast<double>(location.longitude), 13.5);
    EXPECT_DOUBLE_EQ(static_cast<double>(location.latitude), 38.25);
}

TEST(GeoParser, ParsesBoundaryCoordinates) {
    auto min_location = ExpectLocation(GeoParser::ParseLocation("-180", "-85.05112878"));
    auto max_location = ExpectLocation(GeoParser::ParseLocation("180", "85.05112878"));

    EXPECT_DOUBLE_EQ(static_cast<double>(min_location.longitude), -180.0);
    EXPECT_DOUBLE_EQ(static_cast<double>(max_location.longitude), 180.0);
}

TEST(GeoParser, RejectsLongitudeOutsideRange) {
    const auto result = GeoParser::ParseLocation("180.1", "0");
    ASSERT_TRUE(std::holds_alternative<Result>(result));
    ExpectParseError(std::get<Result>(result), "invalid longitude or latitude");
}

TEST(GeoParser, RejectsLatitudeOutsideRange) {
    const auto result = GeoParser::ParseLocation("0", "85.1");
    ASSERT_TRUE(std::holds_alternative<Result>(result));
    ExpectParseError(std::get<Result>(result), "invalid longitude or latitude");
}

TEST(GeoParser, RejectsNonNumericLocationParts) {
    auto bad_longitude = GeoParser::ParseLocation("x", "0");
    auto bad_latitude = GeoParser::ParseLocation("0", "y");

    ASSERT_TRUE(std::holds_alternative<Result>(bad_longitude));
    ASSERT_TRUE(std::holds_alternative<Result>(bad_latitude));
    ExpectParseError(std::get<Result>(bad_longitude), "invalid longitude or latitude");
    ExpectParseError(std::get<Result>(bad_latitude), "invalid longitude or latitude");
}

TEST(GeoParser, RejectsPartiallyParsedCoordinates) {
    const auto result = GeoParser::ParseLocation("10abc", "20");

    ASSERT_TRUE(std::holds_alternative<Result>(result));
    ExpectParseError(std::get<Result>(result), "invalid longitude or latitude");
}

TEST(GeoParser, ParsesSupportedUnitsCaseInsensitively) {
    EXPECT_DOUBLE_EQ(ExpectUnit(GeoParser::ParseUnit("m")), 1000.0);
    EXPECT_DOUBLE_EQ(ExpectUnit(GeoParser::ParseUnit("KM")), 1.0);
    EXPECT_DOUBLE_EQ(ExpectUnit(GeoParser::ParseUnit("Mi")), 0.621371192);
    EXPECT_DOUBLE_EQ(ExpectUnit(GeoParser::ParseUnit("ft")), 3280.839895);
}

TEST(GeoParser, RejectsUnsupportedUnit) {
    const auto result = GeoParser::ParseUnit("yard");

    ASSERT_TRUE(std::holds_alternative<Result>(result));
    ExpectParseError(std::get<Result>(result), "unsupported unit");
}

TEST(GeoParser, ParsesRequiredSearchArguments) {
    const auto search = ExpectSearchArgs(GeoParser::ParseSearch(Args({
        "FROMLONLAT", "15", "37", "BYRADIUS", "100", "km"
    })));

    EXPECT_DOUBLE_EQ(static_cast<double>(search.center.longitude), 15.0);
    EXPECT_DOUBLE_EQ(static_cast<double>(search.center.latitude), 37.0);
    EXPECT_DOUBLE_EQ(search.radius, 100.0);
    EXPECT_DOUBLE_EQ(search.unit_multiplier, 1.0);
    EXPECT_FALSE(search.sort_asc.has_value());
    EXPECT_FALSE(search.count.has_value());
}

TEST(GeoParser, ParsesSearchKeywordsCaseInsensitively) {
    const auto search = ExpectSearchArgs(GeoParser::ParseSearch(Args({
        "fromlonlat", "15", "37", "byradius", "100", "KM"
    })));

    EXPECT_DOUBLE_EQ(search.radius, 100.0);
    EXPECT_DOUBLE_EQ(search.unit_multiplier, 1.0);
}

TEST(GeoParser, ParsesSearchWithAscAndCount) {
    const auto search = ExpectSearchArgs(GeoParser::ParseSearch(Args({
        "FROMLONLAT", "15", "37", "BYRADIUS", "100", "km", "ASC", "COUNT", "3"
    })));

    ASSERT_TRUE(search.sort_asc.has_value());
    EXPECT_TRUE(search.sort_asc.value());
    ASSERT_TRUE(search.count.has_value());
    EXPECT_EQ(search.count.value(), 3);
}

TEST(GeoParser, ParsesSearchWithDescAndCountInReverseOrder) {
    const auto search = ExpectSearchArgs(GeoParser::ParseSearch(Args({
        "FROMLONLAT", "15", "37", "BYRADIUS", "100", "km", "COUNT", "2", "DESC"
    })));

    ASSERT_TRUE(search.sort_asc.has_value());
    EXPECT_FALSE(search.sort_asc.value());
    ASSERT_TRUE(search.count.has_value());
    EXPECT_EQ(search.count.value(), 2);
}

TEST(GeoParser, RejectsSearchWithTooFewArguments) {
    const auto result = GeoParser::ParseSearch(Args({"FROMLONLAT", "15"}));

    ASSERT_TRUE(std::holds_alternative<Result>(result));
    ExpectParseError(std::get<Result>(result), "wrong number of arguments");
}

TEST(GeoParser, RejectsSearchWithWrongPrefix) {
    const auto result = GeoParser::ParseSearch(Args({
        "FROMMEMBER", "15", "37", "BYRADIUS", "100", "km"
    }));

    ASSERT_TRUE(std::holds_alternative<Result>(result));
    ExpectParseError(std::get<Result>(result), "syntax error");
}

TEST(GeoParser, RejectsSearchWithWrongRadiusKeyword) {
    const auto result = GeoParser::ParseSearch(Args({
        "FROMLONLAT", "15", "37", "BYBOX", "100", "km"
    }));

    ASSERT_TRUE(std::holds_alternative<Result>(result));
    ExpectParseError(std::get<Result>(result), "syntax error");
}

TEST(GeoParser, RejectsSearchWithInvalidRadius) {
    const auto result = GeoParser::ParseSearch(Args({
        "FROMLONLAT", "15", "37", "BYRADIUS", "bad", "km"
    }));

    ASSERT_TRUE(std::holds_alternative<Result>(result));
    ExpectParseError(std::get<Result>(result), "invalid radius");
}

TEST(GeoParser, RejectsSearchWithNegativeRadius) {
    const auto result = GeoParser::ParseSearch(Args({
        "FROMLONLAT", "15", "37", "BYRADIUS", "-1", "km"
    }));

    ASSERT_TRUE(std::holds_alternative<Result>(result));
    ExpectParseError(std::get<Result>(result), "invalid radius");
}

TEST(GeoParser, RejectsDuplicateSort) {
    const auto result = GeoParser::ParseSearch(Args({
        "FROMLONLAT", "15", "37", "BYRADIUS", "100", "km", "ASC", "DESC"
    }));

    ASSERT_TRUE(std::holds_alternative<Result>(result));
    ExpectParseError(std::get<Result>(result), "syntax error");
}

TEST(GeoParser, RejectsDuplicateCount) {
    const auto result = GeoParser::ParseSearch(Args({
        "FROMLONLAT", "15", "37", "BYRADIUS", "100", "km", "COUNT", "1", "COUNT", "2"
    }));

    ASSERT_TRUE(std::holds_alternative<Result>(result));
    ExpectParseError(std::get<Result>(result), "syntax error");
}

TEST(GeoParser, RejectsMissingCountValue) {
    const auto result = GeoParser::ParseSearch(Args({
        "FROMLONLAT", "15", "37", "BYRADIUS", "100", "km", "COUNT"
    }));

    ASSERT_TRUE(std::holds_alternative<Result>(result));
    ExpectParseError(std::get<Result>(result), "syntax error");
}

TEST(GeoParser, RejectsZeroCount) {
    const auto result = GeoParser::ParseSearch(Args({
        "FROMLONLAT", "15", "37", "BYRADIUS", "100", "km", "COUNT", "0"
    }));

    ASSERT_TRUE(std::holds_alternative<Result>(result));
    ExpectParseError(std::get<Result>(result), "invalid count");
}

TEST(GeoParser, RejectsUnknownSearchOption) {
    const auto result = GeoParser::ParseSearch(Args({
        "FROMLONLAT", "15", "37", "BYRADIUS", "100", "km", "WITHDIST"
    }));

    ASSERT_TRUE(std::holds_alternative<Result>(result));
    ExpectParseError(std::get<Result>(result), "syntax error");
}
