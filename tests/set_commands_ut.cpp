#include "result_test_utils.hpp"

#include <database/database.hpp>

#include <gtest/gtest.h>

#include <vector>

TEST(SetCommands, AddMembersKeepsOnlyUniqueValues) {
    DataBase db;
    auto values = Args({"one", "two", "one"});

    ExpectInteger(db.Sadd("numbers", values), 2);
    ExpectInteger(db.Scard("numbers"), 2);
    ExpectInteger(db.Sismember("numbers", "one"), 1);
    ExpectInteger(db.Sismember("numbers", "missing"), 0);
    ExpectArray(db.Smembers("numbers"), {"one", "two"});
}

TEST(SetCommands, RemoveAndMoveMembers) {
    DataBase db;
    ExpectInteger(db.Sadd("source", Args({"a", "b"})), 2);

    ExpectInteger(db.Srem("source", Args({"missing", "a"})), 1);
    ExpectArray(db.Smembers("source"), {"b"});

    ExpectInteger(db.Smove("source", "destination", "b"), 1);
    ExpectArray(db.Smembers("source"), {});
    ExpectArray(db.Smembers("destination"), {"b"});
}

TEST(SetCommands, UnionDifferenceAndIntersection) {
    DataBase db;
    ExpectInteger(db.Sadd("first", Args({"a", "b", "c"})), 3);
    ExpectInteger(db.Sadd("second", Args({"b", "c", "d"})), 3);
    ExpectInteger(db.Sadd("third", Args({"c", "d", "e"})), 3);

    ExpectArray(db.Sunion(Args({"first", "second", "third"})), {"a", "b", "c", "d", "e"});
    ExpectArray(db.Sdiff("first", Args({"second"})), {"a"});
    ExpectArray(db.Sinter("first", Args({"second", "third"})), {"c"});
}

TEST(SetCommands, IntersectionWithMissingSetIsEmpty) {
    DataBase db;
    ExpectInteger(db.Sadd("first", Args({"a", "b"})), 2);

    ExpectArray(db.Sinter("first", Args({"missing"})), {});
}

TEST(SetCommands, MoveDoesNotCreateDestinationWhenSourceMissing) {
    DataBase db;

    ExpectInteger(db.Smove("missing", "destination", "member"), 0);
    ExpectInteger(db.Exists(Args({"destination"})), 0);
}

TEST(SetCommands, MissingKeyAndWrongTypeResultsMatchCommandFamily) {
    DataBase db;

    ExpectArray(db.Smembers("missing"), {});
    ExpectInteger(db.Scard("missing"), 0);
    ExpectInteger(db.Sismember("missing", "value"), 0);
    ExpectInteger(db.Srem("missing", Args({"value"})), 0);

    ExpectDone(db.Set("plain", "value"));
    ExpectError(db.Sadd("plain", Args({"x"})), "wrong type");
    ExpectError(db.Smembers("plain"), "wrong type");
}

TEST(SetCommands, AddWrongTypeWinsOverMaxMemoryLimit) {
    DataBase db;

    ExpectDone(db.Set("plain", "value"));
    ExpectDone(db.ConfigSetMaxMemory(1));

    ExpectError(db.Sadd("plain", Args({"x"})), "wrong type");
    ExpectString(db.Get("plain"), "value");
}

TEST(SetCommands, MoveOomDoesNotCreateDestinationOrRemoveSourceMember) {
    DataBase db;

    ExpectInteger(db.Sadd("source", Args({"a"})), 1);
    ExpectDone(db.ConfigSetMaxMemory(1));

    ExpectError(
        db.Smove("source", "destination", "a"),
        "OOM command not allowed when used memory > 'maxmemory'"
    );
    ExpectArray(db.Smembers("source"), {"a"});
    ExpectString(db.Type("destination"), "none");
}

TEST(SetCommands, AddEmptyMemberAndDuplicateEmptyMember) {
    DataBase db;

    ExpectInteger(db.Sadd("set", std::vector<std::string>{"", ""}), 1);
    ExpectInteger(db.Scard("set"), 1);
    ExpectInteger(db.Sismember("set", ""), 1);
}

TEST(SetCommands, RemoveMultipleExistingMembers) {
    DataBase db;

    ExpectInteger(db.Sadd("set", Args({"a", "b", "c"})), 3);
    ExpectInteger(db.Srem("set", Args({"a", "c"})), 2);
    ExpectArray(db.Smembers("set"), {"b"});
}

TEST(SetCommands, RemovingLastMemberDeletesKey) {
    DataBase db;

    ExpectInteger(db.Sadd("set", Args({"a"})), 1);
    ExpectInteger(db.Srem("set", Args({"a"})), 1);
    ExpectString(db.Type("set"), "none");
}

TEST(SetCommands, SmembersReturnsSortedOrderFromStdSet) {
    DataBase db;

    ExpectInteger(db.Sadd("set", Args({"c", "a", "b"})), 3);
    ExpectArray(db.Smembers("set"), {"a", "b", "c"});
}

TEST(SetCommands, SunionIgnoresMissingKeys) {
    DataBase db;

    ExpectInteger(db.Sadd("set", Args({"a"})), 1);
    ExpectArray(db.Sunion(Args({"missing", "set"})), {"a"});
}

TEST(SetCommands, SunionOfOnlyMissingKeysIsEmpty) {
    DataBase db;

    ExpectArray(db.Sunion(Args({"a", "b"})), {});
}

TEST(SetCommands, SdiffIgnoresMissingKeys) {
    DataBase db;

    ExpectInteger(db.Sadd("set", Args({"a", "b"})), 2);
    ExpectArray(db.Sdiff("set", Args({"missing"})), {"a", "b"});
}

TEST(SetCommands, SdiffWithMultipleSetsRemovesAllMatches) {
    DataBase db;

    ExpectInteger(db.Sadd("first", Args({"a", "b", "c", "d"})), 4);
    ExpectInteger(db.Sadd("second", Args({"b"})), 1);
    ExpectInteger(db.Sadd("third", Args({"c", "d"})), 2);
    ExpectArray(db.Sdiff("first", Args({"second", "third"})), {"a"});
}

TEST(SetCommands, SinterWithNoOtherKeysReturnsOriginalSet) {
    DataBase db;

    ExpectInteger(db.Sadd("set", Args({"b", "a"})), 2);
    ExpectArray(db.Sinter("set", {}), {"a", "b"});
}

TEST(SetCommands, SdiffWithNoOtherKeysReturnsOriginalSet) {
    DataBase db;

    ExpectInteger(db.Sadd("set", Args({"b", "a"})), 2);
    ExpectArray(db.Sdiff("set", {}), {"a", "b"});
}

TEST(SetCommands, SmoveToExistingDestinationKeepsUniqueMembers) {
    DataBase db;

    ExpectInteger(db.Sadd("source", Args({"a"})), 1);
    ExpectInteger(db.Sadd("destination", Args({"a"})), 1);
    ExpectInteger(db.Smove("source", "destination", "a"), 1);
    ExpectArray(db.Smembers("source"), {});
    ExpectArray(db.Smembers("destination"), {"a"});
}

TEST(SetCommands, SmoveMissingMemberDoesNotCreateDestination) {
    DataBase db;

    ExpectInteger(db.Sadd("source", Args({"a"})), 1);
    ExpectInteger(db.Smove("source", "destination", "missing"), 0);
    ExpectInteger(db.Exists(Args({"destination"})), 0);
}

TEST(SetCommands, SmoveWrongTypeSourceDoesNotCreateDestination) {
    DataBase db;

    ExpectDone(db.Set("source", "value"));
    ExpectError(db.Smove("source", "destination", "value"), "wrong type");
    ExpectInteger(db.Exists(Args({"destination"})), 0);
}

TEST(SetCommands, SmoveWrongTypeDestinationDoesNotRemoveSourceMember) {
    DataBase db;

    ExpectInteger(db.Sadd("source", Args({"a"})), 1);
    ExpectDone(db.Set("destination", "value"));
    ExpectError(db.Smove("source", "destination", "a"), "wrong type");
    ExpectArray(db.Smembers("source"), {"a"});
}

TEST(SetCommands, SetOperationsRejectListValues) {
    DataBase db;

    ExpectInteger(db.Rpush("list", Args({"x"})), 1);
    ExpectError(db.Sadd("list", Args({"y"})), "wrong type");
    ExpectError(db.Smembers("list"), "wrong type");
    ExpectError(db.Scard("list"), "wrong type");
    ExpectError(db.Sismember("list", "x"), "wrong type");
    ExpectError(db.Srem("list", Args({"x"})), "wrong type");
}

TEST(SetCommands, SetOperationsRejectGeoValues) {
    DataBase db;

    ExpectInteger(db.Geoadd("geo", Args({"10", "10", "p"})), 1);
    ExpectError(db.Sadd("geo", Args({"x"})), "wrong type");
    ExpectError(db.Smembers("geo"), "wrong type");
}

TEST(SetCommands, UnionIntersectionDifferenceRejectWrongTypeOperands) {
    DataBase db;

    ExpectInteger(db.Sadd("set", Args({"a"})), 1);
    ExpectDone(db.Set("plain", "value"));

    ExpectError(db.Sunion(Args({"set", "plain"})), "wrong type");
    ExpectError(db.Sinter("set", Args({"plain"})), "wrong type");
    ExpectError(db.Sdiff("set", Args({"plain"})), "wrong type");
}

TEST(SetCommands, DelRemovesSetKey) {
    DataBase db;

    ExpectInteger(db.Sadd("set", Args({"a"})), 1);
    ExpectInteger(db.Del(Args({"set"})), 1);
    ExpectInteger(db.Scard("set"), 0);
}

TEST(SetCommands, SetCommandOverwritesSetValue) {
    DataBase db;

    ExpectInteger(db.Sadd("set", Args({"a"})), 1);
    ExpectDone(db.Set("set", "plain"));
    ExpectString(db.Type("set"), "string");
    ExpectString(db.Get("set"), "plain");
}

TEST(SetEquivalenceClasses, AddDuplicateRemoveMissingAndRemoveLast) {
    DataBase db;
    ExpectInteger(db.Sadd("members", Args({"a", "a", "b"})), 2);
    ExpectInteger(db.Sadd("members", Args({"a"})), 0);
    ExpectInteger(db.Srem("members", Args({"missing", "a", "a"})), 1);
    ExpectInteger(db.Scard("members"), 1);
    ExpectInteger(db.Srem("members", Args({"b"})), 1);
    ExpectString(db.Type("members"), "none");
    ExpectArray(db.Smembers("members"), {});
}

TEST(SetEquivalenceClasses, SetAlgebraCoversEmptySingleAndOverlappingOperands) {
    DataBase db;
    ExpectInteger(db.Sadd("left", Args({"a", "b"})), 2);
    ExpectInteger(db.Sadd("right", Args({"b", "c"})), 2);
    ExpectArray(db.Sunion(Args({"left"})), {"a", "b"});
    ExpectArray(db.Sunion(Args({"missing", "missing2"})), {});
    ExpectArray(db.Sdiff("left", Args({})), {"a", "b"});
    ExpectArray(db.Sdiff("left", Args({"right"})), {"a"});
    ExpectArray(db.Sdiff("missing", Args({"right"})), {});
    ExpectArray(db.Sinter("left", Args({})), {"a", "b"});
    ExpectArray(db.Sinter("left", Args({"right"})), {"b"});
    ExpectArray(db.Sinter("left", Args({"missing"})), {});
}
