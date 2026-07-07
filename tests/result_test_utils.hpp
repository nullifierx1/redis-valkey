#pragma once

#include <types/result.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

inline std::vector<std::string> Args(std::initializer_list<const char*> values) {
    return std::vector<std::string>(values.begin(), values.end());
}

inline void ExpectDone(const Result& result) {
    EXPECT_NE(std::get_if<DoneResult>(&result.Value()), nullptr);
}

inline void ExpectNil(const Result& result) {
    EXPECT_NE(std::get_if<NilResult>(&result.Value()), nullptr);
}

inline void ExpectError(const Result& result, const std::string& message) {
    const auto* error = std::get_if<ErrorResult>(&result.Value());
    ASSERT_NE(error, nullptr);
    EXPECT_EQ(error->message, message);
}

inline void ExpectInteger(const Result& result, int64_t value) {
    const auto* integer = std::get_if<int64_t>(&result.Value());
    ASSERT_NE(integer, nullptr);
    EXPECT_EQ(*integer, value);
}

inline int64_t GetInteger(const Result& result) {
    const auto* integer = std::get_if<int64_t>(&result.Value());
    EXPECT_NE(integer, nullptr);
    return integer == nullptr ? 0 : *integer;
}

inline void ExpectString(const Result& result, const std::string& value) {
    const auto* string = std::get_if<std::string>(&result.Value());
    const auto* simple_string = std::get_if<SimpleStringResult>(&result.Value());
    ASSERT_TRUE(string != nullptr || simple_string != nullptr);
    EXPECT_EQ(string == nullptr ? simple_string->value : *string, value);
}

inline void ExpectDistanceMetersNear(
    const Result& result,
    double expected_meters,
    double epsilon_meters,
    double unit_to_meters = 1.0
) {
    const auto* string = std::get_if<std::string>(&result.Value());
    ASSERT_NE(string, nullptr);

    double actual = 0.0;
    ASSERT_NO_THROW(actual = std::stod(*string));
    EXPECT_NEAR(actual * unit_to_meters, expected_meters, epsilon_meters);
}

inline void ExpectArray(const Result& result, std::vector<std::string> values) {
    const auto* array = std::get_if<ArrayResult>(&result.Value());
    ASSERT_NE(array, nullptr);
    EXPECT_EQ(array->values, values);
}

inline void ExpectArrayUnordered(
    const Result& result,
    std::vector<std::string> values
) {
    const auto* array = std::get_if<ArrayResult>(&result.Value());
    ASSERT_NE(array, nullptr);

    auto actual = array->values;
    std::sort(actual.begin(), actual.end());
    std::sort(values.begin(), values.end());
    EXPECT_EQ(actual, values);
}

inline GeoPositionResult ExpectGeoPositions(
    const Result& result,
    size_t size
) {
    const auto* positions = std::get_if<GeoPositionResult>(&result.Value());
    EXPECT_NE(positions, nullptr);
    EXPECT_EQ(positions->values.size(), size);
    return *positions;
}
