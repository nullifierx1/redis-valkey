#include "result_test_utils.hpp"

#include <database/database.hpp>

#include <gtest/gtest.h>

#include <cstdint>

TEST(StringCommands, SetGetAndOverwriteValue) {
    DataBase db;

    ExpectDone(db.Set("name", "alice"));
    ExpectString(db.Get("name"), "alice");

    ExpectDone(db.Set("name", "bob"));
    ExpectString(db.Get("name"), "bob");
}

TEST(StringCommands, MissingKeyBehavesLikeValkeyForGetAndStrlen) {
    DataBase db;

    ExpectNil(db.Get("missing"));
    ExpectInteger(db.Strlen("missing"), 0);
}

TEST(StringCommands, AppendCreatesAndExtendsString) {
    DataBase db;

    ExpectInteger(db.Append("message", "hello"), 5);
    ExpectInteger(db.Append("message", " world"), 11);
    ExpectString(db.Get("message"), "hello world");
    ExpectInteger(db.Strlen("message"), 11);
}

TEST(StringCommands, ExpireAndTtlFollowValkeyStyleReturnCodesForStrings) {
    DataBase db;

    ExpectInteger(db.Ttl("missing"), -2);
    ExpectInteger(db.Expire("missing", 10), 0);

    ExpectDone(db.Set("session", "token"));
    ExpectInteger(db.Ttl("session"), -1);
    ExpectInteger(db.Expire("session", 10), 1);

    const int64_t ttl = GetInteger(db.Ttl("session"));
    EXPECT_GE(ttl, 0);
    EXPECT_LE(ttl, 10);
}

TEST(StringCommands, SetClearsTtlAndAppendPreservesTtl) {
    DataBase db;

    ExpectDone(db.Set("key", "value"));
    ExpectInteger(db.Expire("key", 10), 1);
    ExpectInteger(db.Append("key", "!"), 6);
    EXPECT_NE(GetInteger(db.Ttl("key")), -1);

    ExpectDone(db.Set("key", "new-value"));
    ExpectInteger(db.Ttl("key"), -1);
}

TEST(StringCommands, ExpireWithZeroSecondsDeletesStringImmediately) {
    DataBase db;

    ExpectDone(db.Set("temporary", "value"));
    ExpectInteger(db.Expire("temporary", 0), 1);
    ExpectNil(db.Get("temporary"));
    ExpectInteger(db.Ttl("temporary"), -2);
}

TEST(StringCommands, GetRejectsListSetAndGeoValues) {
    DataBase db;

    ExpectInteger(db.Rpush("list", Args({"x"})), 1);
    ExpectInteger(db.Sadd("set", Args({"x"})), 1);
    ExpectInteger(db.Geoadd("geo", Args({"10", "10", "p"})), 1);

    ExpectError(db.Get("list"), "wrong type");
    ExpectError(db.Get("set"), "wrong type");
    ExpectError(db.Get("geo"), "wrong type");
}

TEST(StringCommands, StrlenRejectsListSetAndGeoValues) {
    DataBase db;

    ExpectInteger(db.Rpush("list", Args({"x"})), 1);
    ExpectInteger(db.Sadd("set", Args({"x"})), 1);
    ExpectInteger(db.Geoadd("geo", Args({"10", "10", "p"})), 1);

    ExpectError(db.Strlen("list"), "wrong type");
    ExpectError(db.Strlen("set"), "wrong type");
    ExpectError(db.Strlen("geo"), "wrong type");
}

TEST(StringCommands, AppendRejectsListSetAndGeoValues) {
    DataBase db;

    ExpectInteger(db.Rpush("list", Args({"x"})), 1);
    ExpectInteger(db.Sadd("set", Args({"x"})), 1);
    ExpectInteger(db.Geoadd("geo", Args({"10", "10", "p"})), 1);

    ExpectError(db.Append("list", "y"), "wrong type");
    ExpectError(db.Append("set", "y"), "wrong type");
    ExpectError(db.Append("geo", "y"), "wrong type");
}

TEST(StringCommands, AppendWrongTypeWinsOverMaxMemoryLimit) {
    DataBase db;

    ExpectInteger(db.Rpush("list", Args({"x"})), 1);
    ExpectDone(db.ConfigSetMaxMemory(1));

    ExpectError(db.Append("list", "y"), "wrong type");
    ExpectString(db.Type("list"), "list");
}

TEST(StringCommands, SetOverwritesListSetAndGeoValues) {
    DataBase db;

    ExpectInteger(db.Rpush("list", Args({"x"})), 1);
    ExpectInteger(db.Sadd("set", Args({"x"})), 1);
    ExpectInteger(db.Geoadd("geo", Args({"10", "10", "p"})), 1);

    ExpectDone(db.Set("list", "plain"));
    ExpectDone(db.Set("set", "plain"));
    ExpectDone(db.Set("geo", "plain"));

    ExpectString(db.Type("list"), "string");
    ExpectString(db.Type("set"), "string");
    ExpectString(db.Type("geo"), "string");
}

TEST(StringCommands, AppendEmptyStringDoesNotChangeValue) {
    DataBase db;

    ExpectDone(db.Set("key", "value"));
    ExpectInteger(db.Append("key", ""), 5);
    ExpectString(db.Get("key"), "value");
}

TEST(StringCommands, SetEmptyStringStoresZeroLengthValue) {
    DataBase db;

    ExpectDone(db.Set("key", ""));
    ExpectString(db.Get("key"), "");
    ExpectInteger(db.Strlen("key"), 0);
}

TEST(StringCommands, SetCanStoreWhitespaceInsideValueWhenCalledDirectly) {
    DataBase db;

    ExpectDone(db.Set("key", "hello world"));
    ExpectString(db.Get("key"), "hello world");
    ExpectInteger(db.Strlen("key"), 11);
}

TEST(StringCommands, ExpireCanBeResetToLongerTtl) {
    DataBase db;

    ExpectDone(db.Set("key", "value"));
    ExpectInteger(db.Expire("key", 1), 1);
    ExpectInteger(db.Expire("key", 20), 1);

    const int64_t ttl = GetInteger(db.Ttl("key"));
    EXPECT_GE(ttl, 0);
    EXPECT_LE(ttl, 20);
}

TEST(StringCommands, ExpireWorksForNonStringTypesToo) {
    DataBase db;

    ExpectInteger(db.Rpush("list", Args({"x"})), 1);
    ExpectInteger(db.Sadd("set", Args({"x"})), 1);
    ExpectInteger(db.Geoadd("geo", Args({"10", "10", "p"})), 1);

    ExpectInteger(db.Expire("list", 10), 1);
    ExpectInteger(db.Expire("set", 10), 1);
    ExpectInteger(db.Expire("geo", 10), 1);

    EXPECT_NE(GetInteger(db.Ttl("list")), -1);
    EXPECT_NE(GetInteger(db.Ttl("set")), -1);
    EXPECT_NE(GetInteger(db.Ttl("geo")), -1);
}

TEST(StringCommands, NegativeExpireDeletesImmediatelyWhenCalledDirectly) {
    DataBase db;

    ExpectDone(db.Set("key", "value"));
    ExpectInteger(db.Expire("key", -5), 1);
    ExpectNil(db.Get("key"));
    ExpectString(db.Type("key"), "none");
}
