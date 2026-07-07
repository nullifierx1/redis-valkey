#include "result_test_utils.hpp"

#include <types/db_value.hpp>
#include <types/result.hpp>
#include <database/item.hpp>

#include <gtest/gtest.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <regex>
#include <string>
#include <string_view>
#include <vector>
#include <span>
#include <variant>

#define private public
#include <database/database.hpp>
#undef private

#include <commands/common_commands.hpp>
#include <commands/list_commands.hpp>

namespace gp_helpers {

std::string ToLower(std::string value);
bool EqualsToken(const std::string& value, std::string_view expected);
std::optional<GeoValue::coordinates_type> ParseCoordinate(const std::string& value);
std::optional<int> ParsePositiveCount(const std::string& text);
bool IsValidLongitude(GeoValue::coordinates_type longitude);
bool IsValidLatitude(GeoValue::coordinates_type latitude);
std::optional<double> ParseUnitMultiplier(const std::string& unit);
bool SetSort(std::optional<bool>& sort_ascending, bool value);
std::optional<Result> ParseSortAndCount(
    std::span<const std::string> args,
    size_t& pos,
    std::optional<bool>& sort_ascending,
    std::optional<int>& count
);
std::optional<Result> ParseSearchPrefix(
    std::span<const std::string> args,
    GeoValue::Location& center,
    double& radius,
    double& unit_multiplier
);

} // namespace gp_helpers

TEST(CommonDetail, ParseSecondsAcceptsUnsignedSeconds) {
    EXPECT_EQ(common_detail::ParseSeconds("0"), std::optional<int64_t>(0));
    EXPECT_EQ(common_detail::ParseSeconds("42"), std::optional<int64_t>(42));
}

TEST(CommonDetail, ParseSecondsRejectsInvalidText) {
    EXPECT_FALSE(common_detail::ParseSeconds("").has_value());
    EXPECT_FALSE(common_detail::ParseSeconds("-1").has_value());
    EXPECT_FALSE(common_detail::ParseSeconds("1s").has_value());
}

TEST(CommonDetail, ParseMemorySizeAcceptsPlainBytesAndSuffixes) {
    EXPECT_EQ(common_detail::ParseMemorySize("0"), std::optional<size_t>(0));
    EXPECT_EQ(common_detail::ParseMemorySize("7b"), std::optional<size_t>(7));
    EXPECT_EQ(common_detail::ParseMemorySize("2kb"), std::optional<size_t>(2048));
    EXPECT_EQ(common_detail::ParseMemorySize("3mb"), std::optional<size_t>(3 * 1024 * 1024));
    EXPECT_EQ(common_detail::ParseMemorySize("1GB"), std::optional<size_t>(1024ULL * 1024ULL * 1024ULL));
}

TEST(CommonDetail, ParseMemorySizeRejectsMissingNumberAndUnknownSuffix) {
    EXPECT_FALSE(common_detail::ParseMemorySize("").has_value());
    EXPECT_FALSE(common_detail::ParseMemorySize("kb").has_value());
    EXPECT_FALSE(common_detail::ParseMemorySize("-1").has_value());
    EXPECT_FALSE(common_detail::ParseMemorySize("1tb").has_value());
}

TEST(CommonDetail, ParseMemorySizeRejectsDigitOverflow) {
    EXPECT_FALSE(common_detail::ParseMemorySize("999999999999999999999999999999999999").has_value());
}

TEST(CommonDetail, ParseMemorySizeRejectsMultiplierOverflow) {
    EXPECT_FALSE(common_detail::ParseMemorySize("9999999999999999999gb").has_value());
}

TEST(ListDetail, ParseCountAcceptsUnsignedCounts) {
    EXPECT_EQ(list_detail::ParseCount("0"), std::optional<size_t>(0));
    EXPECT_EQ(list_detail::ParseCount("123"), std::optional<size_t>(123));
}

TEST(ListDetail, ParseCountRejectsInvalidCounts) {
    EXPECT_FALSE(list_detail::ParseCount("").has_value());
    EXPECT_FALSE(list_detail::ParseCount("-1").has_value());
    EXPECT_FALSE(list_detail::ParseCount("1x").has_value());
}

TEST(ListDetail, ParseIndexAcceptsPositiveNegativeAndZero) {
    EXPECT_EQ(list_detail::ParseIndex("0"), std::optional<int64_t>(0));
    EXPECT_EQ(list_detail::ParseIndex("12"), std::optional<int64_t>(12));
    EXPECT_EQ(list_detail::ParseIndex("-3"), std::optional<int64_t>(-3));
}

TEST(ListDetail, ParseIndexRejectsInvalidIndexes) {
    EXPECT_FALSE(list_detail::ParseIndex("").has_value());
    EXPECT_FALSE(list_detail::ParseIndex("-").has_value());
    EXPECT_FALSE(list_detail::ParseIndex("+1").has_value());
    EXPECT_FALSE(list_detail::ParseIndex("1.5").has_value());
}

TEST(ListDetail, ParseInsertPositionAcceptsOnlyUpperAndLowerCaseWords) {
    EXPECT_EQ(list_detail::ParseInsertPosition("BEFORE"), std::optional<bool>(true));
    EXPECT_EQ(list_detail::ParseInsertPosition("before"), std::optional<bool>(true));
    EXPECT_EQ(list_detail::ParseInsertPosition("AFTER"), std::optional<bool>(false));
    EXPECT_EQ(list_detail::ParseInsertPosition("after"), std::optional<bool>(false));
    EXPECT_FALSE(list_detail::ParseInsertPosition("Before").has_value());
    EXPECT_FALSE(list_detail::ParseInsertPosition("MIDDLE").has_value());
}

TEST(GeoParserHelpers, ToLowerConvertsAsciiLettersOnly) {
    EXPECT_EQ(gp_helpers::ToLower("KmMIft"), "kmmift");
    EXPECT_EQ(gp_helpers::ToLower("123_m"), "123_m");
}

TEST(GeoParserHelpers, EqualsTokenIsCaseInsensitiveAgainstExpectedUppercase) {
    EXPECT_TRUE(gp_helpers::EqualsToken("fromlonlat", "FROMLONLAT"));
    EXPECT_TRUE(gp_helpers::EqualsToken("ASC", "ASC"));
    EXPECT_FALSE(gp_helpers::EqualsToken("ASCENDING", "ASC"));
    EXPECT_FALSE(gp_helpers::EqualsToken("ASX", "ASC"));
}

TEST(GeoParserHelpers, ParseCoordinateAcceptsFiniteFullStringNumbers) {
    auto integer = gp_helpers::ParseCoordinate("10");
    auto decimal = gp_helpers::ParseCoordinate("-10.5");

    ASSERT_TRUE(integer.has_value());
    ASSERT_TRUE(decimal.has_value());
    EXPECT_DOUBLE_EQ(static_cast<double>(*integer), 10.0);
    EXPECT_DOUBLE_EQ(static_cast<double>(*decimal), -10.5);
}

TEST(GeoParserHelpers, ParseCoordinateRejectsInvalidPartialAndInfiniteValues) {
    EXPECT_FALSE(gp_helpers::ParseCoordinate("").has_value());
    EXPECT_FALSE(gp_helpers::ParseCoordinate("10x").has_value());
    EXPECT_FALSE(gp_helpers::ParseCoordinate("1e9999").has_value());
}

TEST(GeoParserHelpers, ParsePositiveCountAcceptsPositiveIntegers) {
    EXPECT_EQ(gp_helpers::ParsePositiveCount("1"), std::optional<int>(1));
    EXPECT_EQ(gp_helpers::ParsePositiveCount("25"), std::optional<int>(25));
}

TEST(GeoParserHelpers, ParsePositiveCountRejectsZeroNegativeAndInvalidText) {
    EXPECT_FALSE(gp_helpers::ParsePositiveCount("0").has_value());
    EXPECT_FALSE(gp_helpers::ParsePositiveCount("-1").has_value());
    EXPECT_FALSE(gp_helpers::ParsePositiveCount("1x").has_value());
}

TEST(GeoParserHelpers, CoordinateRangeValidatorsAcceptBoundaries) {
    EXPECT_TRUE(gp_helpers::IsValidLongitude(-180));
    EXPECT_TRUE(gp_helpers::IsValidLongitude(180));
    EXPECT_FALSE(gp_helpers::IsValidLongitude(-180.1L));
    EXPECT_FALSE(gp_helpers::IsValidLongitude(180.1L));

    EXPECT_TRUE(gp_helpers::IsValidLatitude(-85.05112878L));
    EXPECT_TRUE(gp_helpers::IsValidLatitude(85.05112878L));
    EXPECT_FALSE(gp_helpers::IsValidLatitude(-85.1L));
    EXPECT_FALSE(gp_helpers::IsValidLatitude(85.1L));
}

TEST(GeoParserHelpers, ParseUnitMultiplierAcceptsSupportedUnits) {
    EXPECT_DOUBLE_EQ(*gp_helpers::ParseUnitMultiplier("m"), 1000.0);
    EXPECT_DOUBLE_EQ(*gp_helpers::ParseUnitMultiplier("km"), 1.0);
    EXPECT_DOUBLE_EQ(*gp_helpers::ParseUnitMultiplier("mi"), 0.621371192);
    EXPECT_DOUBLE_EQ(*gp_helpers::ParseUnitMultiplier("ft"), 3280.839895);
    EXPECT_FALSE(gp_helpers::ParseUnitMultiplier("yard").has_value());
}

TEST(GeoParserHelpers, SetSortSetsOnlyOnce) {
    std::optional<bool> sort;

    EXPECT_TRUE(gp_helpers::SetSort(sort, true));
    ASSERT_TRUE(sort.has_value());
    EXPECT_TRUE(sort.value());
    EXPECT_FALSE(gp_helpers::SetSort(sort, false));
    EXPECT_TRUE(sort.value());
}

TEST(GeoParserHelpers, ParseSortAndCountConsumesAllOptions) {
    size_t pos = 0;
    std::optional<bool> sort;
    std::optional<int> count;
    auto args = Args({"DESC", "COUNT", "2"});

    auto error = gp_helpers::ParseSortAndCount(args, pos, sort, count);

    EXPECT_FALSE(error.has_value());
    EXPECT_EQ(pos, args.size());
    ASSERT_TRUE(sort.has_value());
    EXPECT_FALSE(sort.value());
    ASSERT_TRUE(count.has_value());
    EXPECT_EQ(count.value(), 2);
}

TEST(GeoParserHelpers, ParseSortAndCountReturnsErrorsForMalformedOptions) {
    size_t pos = 0;
    std::optional<bool> sort;
    std::optional<int> count;
    auto args = Args({"COUNT"});

    auto error = gp_helpers::ParseSortAndCount(args, pos, sort, count);

    ASSERT_TRUE(error.has_value());
    ExpectError(error.value(), "syntax error");
}

TEST(GeoParserHelpers, ParseSearchPrefixExtractsCenterRadiusAndUnit) {
    GeoValue::Location center;
    double radius = 0.0;
    double unit_multiplier = 0.0;
    auto args = Args({"FROMLONLAT", "10", "20", "BYRADIUS", "30", "km"});

    auto error = gp_helpers::ParseSearchPrefix(args, center, radius, unit_multiplier);

    EXPECT_FALSE(error.has_value());
    EXPECT_DOUBLE_EQ(static_cast<double>(center.longitude), 10.0);
    EXPECT_DOUBLE_EQ(static_cast<double>(center.latitude), 20.0);
    EXPECT_DOUBLE_EQ(radius, 30.0);
    EXPECT_DOUBLE_EQ(unit_multiplier, 1.0);
}

TEST(GeoParserHelpers, ParseSearchPrefixRejectsMalformedPrefix) {
    GeoValue::Location center;
    double radius = 0.0;
    double unit_multiplier = 0.0;
    auto args = Args({"FROMMEMBER", "10", "20", "BYRADIUS", "30", "km"});

    auto error = gp_helpers::ParseSearchPrefix(args, center, radius, unit_multiplier);

    ASSERT_TRUE(error.has_value());
    ExpectError(error.value(), "syntax error");
}

TEST(DataBasePrivate, FindItemReturnsExistingItemAndNullForMissing) {
    DataBase db;

    ExpectDone(db.Set("key", "value"));

    EXPECT_NE(db.FindItem("key"), nullptr);
    EXPECT_EQ(db.FindItem("missing"), nullptr);
}

TEST(DataBasePrivate, ConstFindItemCleansExpiredItem) {
    DataBase db;
    ExpectDone(db.Set("key", "value"));
    ExpectInteger(db.Expire("key", 0), 1);

    const DataBase& const_db = db;
    EXPECT_EQ(const_db.FindItem("key"), nullptr);
    EXPECT_EQ(db.data_.count("key"), 0U);
}

TEST(DataBasePrivate, GetOrCreateValueCreatesRequestedType) {
    DataBase db;

    auto* value = db.GetOrCreateValue<ListValue>("list");

    ASSERT_NE(value, nullptr);
    EXPECT_TRUE(value->data.empty());
    ExpectString(db.Type("list"), "list");
}

TEST(DataBasePrivate, GetOrCreateValueReturnsNullForWrongExistingType) {
    DataBase db;

    ExpectDone(db.Set("key", "value"));

    EXPECT_EQ(db.GetOrCreateValue<ListValue>("key"), nullptr);
    ExpectString(db.Type("key"), "string");
}

TEST(DataBasePrivate, GetOrCreateValueRecreatesExpiredKey) {
    DataBase db;

    ExpectDone(db.Set("key", "value"));
    ExpectInteger(db.Expire("key", 0), 1);

    auto* set = db.GetOrCreateValue<SetValue>("key");

    ASSERT_NE(set, nullptr);
    EXPECT_TRUE(set->data.empty());
    ExpectString(db.Type("key"), "set");
}

TEST(DataBasePrivate, MakeRegexPatternTranslatesWildcardsAndEscapesRegexCharacters) {
    const std::regex wildcard(DataBase::MakeRegexPattern("user:?"));
    EXPECT_TRUE(std::regex_match("user:1", wildcard));
    EXPECT_FALSE(std::regex_match("user:12", wildcard));

    const std::regex literal_dot(DataBase::MakeRegexPattern("file.txt"));
    EXPECT_TRUE(std::regex_match("file.txt", literal_dot));
    EXPECT_FALSE(std::regex_match("file-txt", literal_dot));
}

TEST(DataBasePrivate, NormalizeListIndexHandlesPositiveAndNegativeIndexes) {
    EXPECT_EQ(DataBase::NormalizeListIndex(0, 3), std::optional<size_t>(0));
    EXPECT_EQ(DataBase::NormalizeListIndex(2, 3), std::optional<size_t>(2));
    EXPECT_EQ(DataBase::NormalizeListIndex(-1, 3), std::optional<size_t>(2));
    EXPECT_EQ(DataBase::NormalizeListIndex(-3, 3), std::optional<size_t>(0));
}

TEST(DataBasePrivate, NormalizeListIndexRejectsOutOfRangeAndEmptyList) {
    EXPECT_FALSE(DataBase::NormalizeListIndex(0, 0).has_value());
    EXPECT_FALSE(DataBase::NormalizeListIndex(3, 3).has_value());
    EXPECT_FALSE(DataBase::NormalizeListIndex(-4, 3).has_value());
}

TEST(DataBasePrivate, IsExpiredRecognizesExpiredAndLiveItems) {
    RedisItem live(Value{StringValue{"value"}});
    RedisItem expired(Value{StringValue{"value"}});
    expired.expired_at = std::chrono::steady_clock::now() - std::chrono::seconds(1);

    EXPECT_FALSE(DataBase::IsExpired(live));
    EXPECT_TRUE(DataBase::IsExpired(expired));
}

TEST(DataBasePrivate, PopOneValueRemovesFromRequestedSide) {
    ListValue list;
    list.data.push_back("a");
    list.data.push_back("b");

    EXPECT_EQ(DataBase::PopOneValue(list, true), "a");
    EXPECT_EQ(DataBase::PopOneValue(list, false), "b");
    EXPECT_TRUE(list.data.empty());
}

TEST(DataBasePrivate, PopManyValuesRemovesOnlyRequestedCount) {
    ListValue list;
    list.data.push_back("a");
    list.data.push_back("b");
    list.data.push_back("c");

    EXPECT_EQ(
        DataBase::PopManyValues(list, 2, true),
        (std::vector<std::string>{"a", "b"})
    );
    ASSERT_EQ(list.data.size(), 1U);
    EXPECT_EQ(list.data.front(), "c");
}

TEST(DataBasePrivate, EraseListIfEmptyRemovesOnlyEmptyLists) {
    DataBase db;
    ExpectInteger(db.Rpush("nonempty", Args({"x"})), 1);
    ExpectInteger(db.Rpush("empty", Args({"x"})), 1);

    auto* nonempty = std::get_if<ListValue>(&db.data_.find("nonempty")->second.value);
    auto* empty = std::get_if<ListValue>(&db.data_.find("empty")->second.value);
    ASSERT_NE(nonempty, nullptr);
    ASSERT_NE(empty, nullptr);
    empty->data.clear();

    db.EraseListIfEmpty("nonempty", *nonempty);
    db.EraseListIfEmpty("empty", *empty);

    EXPECT_EQ(db.data_.count("nonempty"), 1U);
    EXPECT_EQ(db.data_.count("empty"), 0U);
}

TEST(DataBasePrivate, ItemMemoryUsageIncludesKeyAndValue) {
    RedisItem item(Value{StringValue{"value"}});
    const std::string long_key(128, 'k');

    EXPECT_GT(DataBase::ItemMemoryUsage("key", item), item.value.index());
    EXPECT_GT(DataBase::ItemMemoryUsage(long_key, item), DataBase::ItemMemoryUsage("k", item));
}

TEST(DataBasePrivate, ValuesMemoryUsageIsZeroForEmptyInput) {
    EXPECT_EQ(DataBase::ValuesMemoryUsage({}), 0U);
}

TEST(DataBasePrivate, ValuesMemoryUsageGrowsWithStoredStrings) {
    auto small = Args({"a"});
    std::vector<std::string> large = {"a", std::string(128, 'x')};

    EXPECT_GT(DataBase::ValuesMemoryUsage(large), DataBase::ValuesMemoryUsage(small));
}

TEST(DataBasePrivate, UsedMemoryGrowsWhenDataIsAdded) {
    DataBase db;
    const size_t empty_usage = db.UsedMemory();

    ExpectDone(db.Set("key", std::string(128, 'x')));

    EXPECT_GT(db.UsedMemory(), empty_usage);
}

TEST(DataBasePrivate, UsedMemoryCleansExpiredKeys) {
    DataBase db;
    ExpectDone(db.Set("key", "value"));
    ExpectInteger(db.Expire("key", 0), 1);

    static_cast<void>(db.UsedMemory());

    EXPECT_EQ(db.data_.count("key"), 0U);
}

TEST(DataBasePrivate, WouldExceedMaxMemoryFalseWhenLimitDisabled) {
    DataBase db;

    EXPECT_FALSE(db.PossibleToAdd(1024));
}

TEST(DataBasePrivate, WouldExceedMaxMemoryDetectsOverLimitWrite) {
    DataBase db;
    db.max_memory_ = db.UsedMemory();

    EXPECT_TRUE(db.PossibleToAdd(1));
}

TEST(DataBasePrivate, PossibleToAddAllowsZeroBytesEvenAtLimit) {
    DataBase db;
    db.max_memory_ = db.UsedMemory();

    EXPECT_FALSE(db.PossibleToAdd(0));
}

TEST(DataBasePrivate, PossibleToAddAcceptsExactRemainingCapacity) {
    DataBase db;
    const size_t used_memory = db.UsedMemory();
    db.max_memory_ = used_memory + 10;

    EXPECT_FALSE(db.PossibleToAdd(10));
    EXPECT_TRUE(db.PossibleToAdd(11));
}

TEST(DataBasePrivate, OomErrorMessageMatchesReadme) {
    ExpectError(
        DataBase::OomError(),
        "OOM command not allowed when used memory > 'maxmemory'"
    );
}
