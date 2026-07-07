#include "result_test_utils.hpp"

#include <database/database.hpp>

#include <gtest/gtest.h>

#include <optional>
#include <vector>

TEST(ListCommands, PushCommandsReturnLengthAndPreserveOrder) {
    DataBase db;

    auto left = Args({"a", "b"});
    auto right = Args({"c", "d"});

    ExpectInteger(db.Lpush("items", left), 2);
    ExpectInteger(db.Rpush("items", right), 4);
    ExpectArray(db.Lrange("items", 0, -1), {"b", "a", "c", "d"});
}

TEST(ListCommands, IndexSetAndInsertUseValkeyStyleIndexes) {
    DataBase db;
    auto values = Args({"one", "two", "three"});
    ExpectInteger(db.Rpush("items", values), 3);

    ExpectString(db.Lindex("items", -1), "three");
    ExpectNil(db.Lindex("items", 10));

    ExpectDone(db.Lset("items", 1, "TWO"));
    ExpectInteger(db.Linsert("items", false, "TWO", "two-and-half"), 4);
    ExpectArray(db.Lrange("items", 0, -1), {"one", "TWO", "two-and-half", "three"});
}

TEST(ListCommands, SetAndInsertReportErrorsForMissingIndexesAndPivots) {
    DataBase db;
    auto values = Args({"one", "two"});
    ExpectInteger(db.Rpush("items", values), 2);

    ExpectError(db.Lset("items", 10, "ten"), "index out of range");
    ExpectError(db.Lset("missing", 0, "zero"), "no such key");
    ExpectInteger(db.Linsert("items", true, "missing-pivot", "value"), -1);
    ExpectInteger(db.Linsert("missing", true, "pivot", "value"), 0);
}

TEST(ListCommands, PopOneAndManyRemoveValuesFromRequestedSide) {
    DataBase db;
    auto values = Args({"one", "two", "three"});
    ExpectInteger(db.Rpush("items", values), 3);

    ExpectString(db.Lpop("items", std::nullopt), "one");
    ExpectArray(db.Rpop("items", 2), {"three", "two"});
    ExpectInteger(db.Llen("items"), 0);
}

TEST(ListCommands, PopWithZeroCountReturnsEmptyArrayWithoutRemoving) {
    DataBase db;
    auto values = Args({"one", "two"});
    ExpectInteger(db.Rpush("items", values), 2);

    ExpectArray(db.Lpop("items", 0), {});
    ExpectArray(db.Lrange("items", 0, -1), {"one", "two"});
}

TEST(ListCommands, MissingKeyAndWrongTypeResultsAreIsolated) {
    DataBase db;

    ExpectInteger(db.Llen("missing"), 0);
    ExpectNil(db.Lpop("missing", std::nullopt));
    ExpectArray(db.Lrange("missing", 0, -1), {});

    ExpectDone(db.Set("plain", "value"));
    ExpectError(db.Llen("plain"), "wrong type");
    ExpectError(db.Rpush("plain", Args({"x"})), "wrong type");
}

TEST(ListCommands, PushWrongTypeWinsOverMaxMemoryLimit) {
    DataBase db;

    ExpectDone(db.Set("plain", "value"));
    ExpectDone(db.ConfigSetMaxMemory(1));

    ExpectError(db.Rpush("plain", Args({"x"})), "wrong type");
    ExpectString(db.Get("plain"), "value");
}

TEST(ListCommands, LpushPrependsEveryArgumentInInputOrder) {
    DataBase db;

    ExpectInteger(db.Lpush("items", Args({"one", "two", "three"})), 3);
    ExpectArray(db.Lrange("items", 0, -1), {"three", "two", "one"});
}

TEST(ListCommands, RpushAppendsEveryArgumentInInputOrder) {
    DataBase db;

    ExpectInteger(db.Rpush("items", Args({"one", "two", "three"})), 3);
    ExpectArray(db.Lrange("items", 0, -1), {"one", "two", "three"});
}

TEST(ListCommands, PushWithEmptyValueStoresEmptyString) {
    DataBase db;

    ExpectInteger(db.Rpush("items", std::vector<std::string>{""}), 1);
    ExpectString(db.Lindex("items", 0), "");
}

TEST(ListCommands, LindexSupportsNegativeIndexesAcrossList) {
    DataBase db;

    ExpectInteger(db.Rpush("items", Args({"a", "b", "c"})), 3);
    ExpectString(db.Lindex("items", -1), "c");
    ExpectString(db.Lindex("items", -2), "b");
    ExpectString(db.Lindex("items", -3), "a");
    ExpectNil(db.Lindex("items", -4));
}

TEST(ListCommands, LsetSupportsNegativeIndex) {
    DataBase db;

    ExpectInteger(db.Rpush("items", Args({"a", "b", "c"})), 3);
    ExpectDone(db.Lset("items", -1, "C"));
    ExpectArray(db.Lrange("items", 0, -1), {"a", "b", "C"});
}

TEST(ListCommands, LsetCanStoreEmptyString) {
    DataBase db;

    ExpectInteger(db.Rpush("items", Args({"a"})), 1);
    ExpectDone(db.Lset("items", 0, ""));
    ExpectString(db.Lindex("items", 0), "");
}

TEST(ListCommands, LinsertBeforePivotAtBeginning) {
    DataBase db;

    ExpectInteger(db.Rpush("items", Args({"b", "c"})), 2);
    ExpectInteger(db.Linsert("items", true, "b", "a"), 3);
    ExpectArray(db.Lrange("items", 0, -1), {"a", "b", "c"});
}

TEST(ListCommands, LinsertAfterPivotAtEnd) {
    DataBase db;

    ExpectInteger(db.Rpush("items", Args({"a", "b"})), 2);
    ExpectInteger(db.Linsert("items", false, "b", "c"), 3);
    ExpectArray(db.Lrange("items", 0, -1), {"a", "b", "c"});
}

TEST(ListCommands, LinsertUsesFirstMatchingPivot) {
    DataBase db;

    ExpectInteger(db.Rpush("items", Args({"a", "b", "b", "c"})), 4);
    ExpectInteger(db.Linsert("items", false, "b", "x"), 5);
    ExpectArray(db.Lrange("items", 0, -1), {"a", "b", "x", "b", "c"});
}

TEST(ListCommands, LrangeSupportsPositiveInclusiveStop) {
    DataBase db;

    ExpectInteger(db.Rpush("items", Args({"a", "b", "c", "d"})), 4);
    ExpectArray(db.Lrange("items", 1, 2), {"b", "c"});
}

TEST(ListCommands, LrangeSupportsNegativeBounds) {
    DataBase db;

    ExpectInteger(db.Rpush("items", Args({"a", "b", "c", "d"})), 4);
    ExpectArray(db.Lrange("items", -3, -2), {"b", "c"});
}

TEST(ListCommands, LrangeClampsOutOfRangeBounds) {
    DataBase db;

    ExpectInteger(db.Rpush("items", Args({"a", "b", "c"})), 3);
    ExpectArray(db.Lrange("items", -100, 100), {"a", "b", "c"});
}

TEST(ListCommands, LrangeReturnsEmptyWhenStartAfterStop) {
    DataBase db;

    ExpectInteger(db.Rpush("items", Args({"a", "b", "c"})), 3);
    ExpectArray(db.Lrange("items", 2, 1), {});
}

TEST(ListCommands, PopCountGreaterThanLengthReturnsAllAndDeletesKey) {
    DataBase db;

    ExpectInteger(db.Rpush("items", Args({"a", "b"})), 2);
    ExpectArray(db.Lpop("items", 10), {"a", "b"});
    ExpectString(db.Type("items"), "none");
}

TEST(ListCommands, RpopCountGreaterThanLengthReturnsReversePopOrder) {
    DataBase db;

    ExpectInteger(db.Rpush("items", Args({"a", "b", "c"})), 3);
    ExpectArray(db.Rpop("items", 10), {"c", "b", "a"});
    ExpectString(db.Type("items"), "none");
}

TEST(ListCommands, PopLastElementDeletesKey) {
    DataBase db;

    ExpectInteger(db.Rpush("items", Args({"a"})), 1);
    ExpectString(db.Lpop("items", std::nullopt), "a");
    ExpectString(db.Type("items"), "none");
}

TEST(ListCommands, ListOperationsRejectSetValues) {
    DataBase db;

    ExpectInteger(db.Sadd("set", Args({"x"})), 1);
    ExpectError(db.Llen("set"), "wrong type");
    ExpectError(db.Lindex("set", 0), "wrong type");
    ExpectError(db.Lset("set", 0, "x"), "wrong type");
    ExpectError(db.Lpop("set", std::nullopt), "wrong type");
    ExpectError(db.Linsert("set", true, "x", "y"), "wrong type");
    ExpectError(db.Lrange("set", 0, -1), "wrong type");
}

TEST(ListCommands, ListOperationsRejectGeoValues) {
    DataBase db;

    ExpectInteger(db.Geoadd("geo", Args({"10", "10", "p"})), 1);
    ExpectError(db.Rpush("geo", Args({"x"})), "wrong type");
    ExpectError(db.Lrange("geo", 0, -1), "wrong type");
}

TEST(ListCommands, SetCommandOverwritesListValue) {
    DataBase db;

    ExpectInteger(db.Rpush("items", Args({"a", "b"})), 2);
    ExpectDone(db.Set("items", "plain"));
    ExpectString(db.Type("items"), "string");
    ExpectString(db.Get("items"), "plain");
}

TEST(ListCommands, DelRemovesListKey) {
    DataBase db;

    ExpectInteger(db.Rpush("items", Args({"a"})), 1);
    ExpectInteger(db.Del(Args({"items"})), 1);
    ExpectInteger(db.Llen("items"), 0);
}
