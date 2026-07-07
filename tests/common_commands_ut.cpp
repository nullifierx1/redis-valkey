#include "result_test_utils.hpp"

#include <database/database.hpp>

#include <cstdint>
#include <gtest/gtest.h>

TEST(CommonCommands, TypeReportsStoredValueKindsAndNone) {
    DataBase db;
    ExpectDone(db.Set("plain", "value"));
    ExpectInteger(db.Rpush("list", Args({"a"})), 1);
    ExpectInteger(db.Sadd("set", Args({"a"})), 1);
    ExpectInteger(db.Geoadd("geo", Args({"13.361389", "38.115556", "Palermo"})), 1);

    ExpectString(db.Type("plain"), "string");
    ExpectString(db.Type("list"), "list");
    ExpectString(db.Type("set"), "set");
    ExpectString(db.Type("geo"), "geo");
    ExpectString(db.Type("missing"), "none");
}

TEST(CommonCommands, ExistsDelDbSizeAndFlushDbOperatePerKey) {
    DataBase db;
    ExpectDone(db.Set("a", "1"));
    ExpectDone(db.Set("b", "2"));

    ExpectInteger(db.DbSize(), 2);
    ExpectInteger(db.Exists(Args({"a", "b", "missing"})), 2);
    ExpectInteger(db.Del(Args({"a", "missing"})), 1);
    ExpectInteger(db.Exists(Args({"a", "b"})), 1);
    ExpectInteger(db.DbSize(), 1);

    ExpectDone(db.FlushDb());
    ExpectInteger(db.DbSize(), 0);
}

TEST(CommonCommands, KeysSupportsStarQuestionAndEscapedRegexChars) {
    DataBase db;
    ExpectDone(db.Set("user:1", "a"));
    ExpectDone(db.Set("user:2", "b"));
    ExpectDone(db.Set("admin:1", "c"));
    ExpectDone(db.Set("file.txt", "d"));
    ExpectDone(db.Set("file-txt", "e"));

    ExpectArrayUnordered(db.Keys("user:*"), {"user:1", "user:2"});
    ExpectArrayUnordered(db.Keys("user:?"), {"user:1", "user:2"});
    ExpectArrayUnordered(db.Keys("file.txt"), {"file.txt"});
}

TEST(CommonCommands, MemoryUsageReportsNilForMissingKeyAndGrowsWithData) {
    DataBase db;

    ExpectNil(db.MemoryUsage("missing"));

    ExpectDone(db.Set("key", "a"));
    const int64_t small_usage = GetInteger(db.MemoryUsage("key"));
    EXPECT_GT(small_usage, 0);

    ExpectInteger(db.Append("key", std::string(256, 'x')), 257);
    const int64_t large_usage = GetInteger(db.MemoryUsage("key"));
    EXPECT_GT(large_usage, small_usage);
}

TEST(CommonCommands, MaxMemoryRejectsGrowingWritesAndRollsBack) {
    DataBase db;
    db.SetMaxMemory(1);

    ExpectError(
        db.Set("key", "value"),
        "OOM command not allowed when used memory > 'maxmemory'"
    );
    ExpectInteger(db.Exists(Args({"key"})), 0);
}

TEST(CommonCommands, ConfigSetAndGetMaxMemory) {
    DataBase db;

    ExpectInteger(db.ConfigGetMaxMemory(), 0);
    ExpectDone(db.ConfigSetMaxMemory(1024));
    ExpectInteger(db.ConfigGetMaxMemory(), 1024);
}

TEST(CommonCommands, TypeReturnsNoneAfterDelete) {
    DataBase db;

    ExpectDone(db.Set("key", "value"));
    ExpectInteger(db.Del(Args({"key"})), 1);
    ExpectString(db.Type("key"), "none");
}

TEST(CommonCommands, DelCountsOnlyExistingKeys) {
    DataBase db;

    ExpectDone(db.Set("a", "1"));
    ExpectDone(db.Set("b", "2"));
    ExpectInteger(db.Del(Args({"a", "missing", "b"})), 2);
    ExpectInteger(db.DbSize(), 0);
}

TEST(CommonCommands, DelSameKeyTwiceCountsOnce) {
    DataBase db;

    ExpectDone(db.Set("a", "1"));
    ExpectInteger(db.Del(Args({"a", "a"})), 1);
}

TEST(CommonCommands, ExistsCountsDuplicateKeysLikeValkey) {
    DataBase db;

    ExpectDone(db.Set("a", "1"));
    ExpectInteger(db.Exists(Args({"a", "a", "missing"})), 2);
}

TEST(CommonCommands, KeysReturnsEmptyForNoMatches) {
    DataBase db;

    ExpectDone(db.Set("alpha", "1"));
    ExpectArray(db.Keys("z*"), {});
}

TEST(CommonCommands, KeysSupportsLiteralRegexMetacharacters) {
    DataBase db;

    ExpectDone(db.Set("a+b", "1"));
    ExpectDone(db.Set("a.b", "2"));
    ExpectDone(db.Set("ab", "3"));

    ExpectArray(db.Keys("a+b"), {"a+b"});
    ExpectArray(db.Keys("a.b"), {"a.b"});
}

TEST(CommonCommands, KeysQuestionMatchesSingleCharacterOnly) {
    DataBase db;

    ExpectDone(db.Set("a1", "1"));
    ExpectDone(db.Set("a12", "2"));
    ExpectArray(db.Keys("a?"), {"a1"});
}

TEST(CommonCommands, FlushDbRemovesAllTypes) {
    DataBase db;

    ExpectDone(db.Set("string", "value"));
    ExpectInteger(db.Rpush("list", Args({"x"})), 1);
    ExpectInteger(db.Sadd("set", Args({"x"})), 1);
    ExpectInteger(db.Geoadd("geo", Args({"10", "10", "p"})), 1);

    ExpectDone(db.FlushDb());
    ExpectInteger(db.DbSize(), 0);
    ExpectString(db.Type("string"), "none");
    ExpectString(db.Type("list"), "none");
    ExpectString(db.Type("set"), "none");
    ExpectString(db.Type("geo"), "none");
}

TEST(CommonCommands, ExpiredKeysDisappearFromDbSize) {
    DataBase db;

    ExpectDone(db.Set("key", "value"));
    ExpectInteger(db.Expire("key", 0), 1);
    ExpectInteger(db.DbSize(), 0);
}

TEST(CommonCommands, ExpiredKeysDisappearFromKeys) {
    DataBase db;

    ExpectDone(db.Set("key", "value"));
    ExpectInteger(db.Expire("key", 0), 1);
    ExpectArray(db.Keys("*"), {});
}

TEST(CommonCommands, MemoryUsageWorksForAllTypes) {
    DataBase db;

    ExpectDone(db.Set("string", "value"));
    ExpectInteger(db.Rpush("list", Args({"x"})), 1);
    ExpectInteger(db.Sadd("set", Args({"x"})), 1);
    ExpectInteger(db.Geoadd("geo", Args({"10", "10", "p"})), 1);

    EXPECT_GT(GetInteger(db.MemoryUsage("string")), 0);
    EXPECT_GT(GetInteger(db.MemoryUsage("list")), 0);
    EXPECT_GT(GetInteger(db.MemoryUsage("set")), 0);
    EXPECT_GT(GetInteger(db.MemoryUsage("geo")), 0);
}

TEST(CommonCommands, MemoryUsageReturnsNilAfterDelete) {
    DataBase db;

    ExpectDone(db.Set("key", "value"));
    ExpectInteger(db.Del(Args({"key"})), 1);
    ExpectNil(db.MemoryUsage("key"));
}

TEST(CommonCommands, MemoryUsageGrowsForListSetAndGeo) {
    DataBase db;

    ExpectInteger(db.Rpush("list", Args({"a"})), 1);
    const int64_t list_small = GetInteger(db.MemoryUsage("list"));
    ExpectInteger(db.Rpush("list", Args({"bbbbbbbbbbbbbbbbbbbb"})), 2);
    EXPECT_GT(GetInteger(db.MemoryUsage("list")), list_small);

    ExpectInteger(db.Sadd("set", Args({"a"})), 1);
    const int64_t set_small = GetInteger(db.MemoryUsage("set"));
    ExpectInteger(db.Sadd("set", Args({"bbbbbbbbbbbbbbbbbbbb"})), 1);
    EXPECT_GT(GetInteger(db.MemoryUsage("set")), set_small);

    ExpectInteger(db.Geoadd("geo", Args({"10", "10", "a"})), 1);
    const int64_t geo_small = GetInteger(db.MemoryUsage("geo"));
    ExpectInteger(db.Geoadd("geo", Args({"11", "11", "bbbbbbbbbbbbbbbbbbbb"})), 1);
    EXPECT_GT(GetInteger(db.MemoryUsage("geo")), geo_small);
}

TEST(CommonCommands, ConfigSetZeroDisablesMaxMemoryLimit) {
    DataBase db;

    ExpectDone(db.ConfigSetMaxMemory(1));
    ExpectDone(db.ConfigSetMaxMemory(0));
    ExpectDone(db.Set("large", std::string(1024, 'x')));
}

TEST(CommonCommands, MaxMemoryAllowsShrinkingWritesWhenAlreadyOverLimit) {
    DataBase db;

    ExpectDone(db.Set("key", std::string(128, 'x')));
    ExpectDone(db.ConfigSetMaxMemory(1));
    ExpectDone(db.Set("key", ""));
    ExpectString(db.Get("key"), "");
}

TEST(CommonCommands, MaxMemoryRejectsAppendAndKeepsOriginalValue) {
    DataBase db;

    ExpectDone(db.Set("key", "value"));
    ExpectDone(db.ConfigSetMaxMemory(1));
    ExpectError(
        db.Append("key", std::string(1024, 'x')),
        "OOM command not allowed when used memory > 'maxmemory'"
    );
    ExpectString(db.Get("key"), "value");
}

TEST(CommonCommands, MaxMemoryRejectsListPushAndKeepsMissingKeyMissing) {
    DataBase db;

    ExpectDone(db.ConfigSetMaxMemory(1));
    ExpectError(
        db.Rpush("list", Args({"value"})),
        "OOM command not allowed when used memory > 'maxmemory'"
    );
    ExpectString(db.Type("list"), "none");
}

TEST(CommonCommands, MaxMemoryRejectsSetAddAndKeepsMissingKeyMissing) {
    DataBase db;

    ExpectDone(db.ConfigSetMaxMemory(1));
    ExpectError(
        db.Sadd("set", Args({"value"})),
        "OOM command not allowed when used memory > 'maxmemory'"
    );
    ExpectString(db.Type("set"), "none");
}

TEST(CommonCommands, MaxMemoryRejectsGeoAddAndKeepsMissingKeyMissing) {
    DataBase db;

    ExpectDone(db.ConfigSetMaxMemory(1));
    ExpectError(
        db.Geoadd("geo", Args({"10", "10", "point"})),
        "OOM command not allowed when used memory > 'maxmemory'"
    );
    ExpectString(db.Type("geo"), "none");
}

TEST(CommonCommands, ConfigSetDoesNotEvictExistingData) {
    DataBase db;

    ExpectDone(db.Set("key", "value"));
    ExpectDone(db.ConfigSetMaxMemory(1));
    ExpectString(db.Get("key"), "value");
}
