#include "result_test_utils.hpp"

#include <types/result.hpp>
#include <database/database.hpp>
#include <handlers/command_handler.hpp>
#include <handlers/parser.hpp>

#include <gtest/gtest.h>

#include <string>
#include <cstdint>

namespace {

Result Execute(DataBase& db, const std::string& line) {
    Parser parser;
    CommandHandler handler;
    return handler.Execute(db, parser.Parse(line));
}

} // namespace

TEST(CommandHandler, EmptyAndUnknownCommandsReturnErrors) {
    DataBase db;
    CommandHandler handler;

    ExpectError(handler.Execute(db, ParsedCommand{}), "empty command");
    ExpectError(Execute(db, "NO_SUCH_COMMAND key"), "unknown command");
}

TEST(CommandHandler, CommandsAreCaseInsensitive) {
    DataBase db;

    ExpectDone(Execute(db, "set key value"));
    ExpectString(Execute(db, "GeT key"), "value");
}

TEST(CommandHandler, ExpireAndTtlCommandsAreRegistered) {
    DataBase db;

    ExpectDone(Execute(db, "SET key value"));
    ExpectInteger(Execute(db, "EXPIRE key 10"), 1);
    const int64_t ttl = GetInteger(Execute(db, "TTL key"));
    EXPECT_GE(ttl, 0);
    EXPECT_LE(ttl, 10);
}

TEST(CommandHandler, CommonCommandsAreRegistered) {
    DataBase db;

    ExpectDone(Execute(db, "SET alpha 1"));
    ExpectInteger(Execute(db, "EXISTS alpha missing"), 1);
    ExpectString(Execute(db, "TYPE alpha"), "string");
    ExpectArray(Execute(db, "KEYS a*"), {"alpha"});
    ExpectInteger(Execute(db, "DBSIZE"), 1);
    ExpectInteger(Execute(db, "DEL alpha"), 1);
    ExpectDone(Execute(db, "FLUSHDB"));
}

TEST(CommandHandler, MultiWordMemoryAndConfigCommandsAreRegistered) {
    DataBase db;

    ExpectDone(Execute(db, "CONFIG SET maxmemory 1kb"));
    ExpectInteger(Execute(db, "CONFIG GET maxmemory"), 1024);

    ExpectDone(Execute(db, "CONFIG SET maxmemory 0"));
    ExpectDone(Execute(db, "SET alpha value"));
    EXPECT_GT(GetInteger(Execute(db, "MEMORY USAGE alpha")), 0);
}

TEST(CommandHandler, ListCommandsAreRegistered) {
    DataBase db;

    ExpectInteger(Execute(db, "LPUSH list b a"), 2);
    ExpectInteger(Execute(db, "RPUSH list c"), 3);
    ExpectInteger(Execute(db, "LLEN list"), 3);
    ExpectString(Execute(db, "LINDEX list 1"), "b");
    ExpectDone(Execute(db, "LSET list 1 B"));
    ExpectInteger(Execute(db, "LINSERT list AFTER B bb"), 4);
    ExpectArray(Execute(db, "LRANGE list 0 -1"), {"a", "B", "bb", "c"});
    ExpectString(Execute(db, "LPOP list"), "a");
    ExpectString(Execute(db, "RPOP list"), "c");
}

TEST(CommandHandler, SetCommandsAreRegistered) {
    DataBase db;

    ExpectInteger(Execute(db, "SADD one a b"), 2);
    ExpectInteger(Execute(db, "SADD two b c"), 2);
    ExpectInteger(Execute(db, "SCARD one"), 2);
    ExpectInteger(Execute(db, "SISMEMBER one a"), 1);
    ExpectArray(Execute(db, "SMEMBERS one"), {"a", "b"});
    ExpectArray(Execute(db, "SINTER one two"), {"b"});
    ExpectArray(Execute(db, "SDIFF one two"), {"a"});
    ExpectArray(Execute(db, "SUNION one two"), {"a", "b", "c"});
    ExpectInteger(Execute(db, "SMOVE one two a"), 1);
    ExpectInteger(Execute(db, "SREM two a"), 1);
}

TEST(CommandHandler, WrongArgumentCountIsReportedByCommand) {
    DataBase db;

    ExpectError(Execute(db, "SET key"), "wrong number of arguments");
    ExpectError(Execute(db, "GEODIST Sicily Palermo"), "wrong number of arguments");
    ExpectError(Execute(db, "EXPIRE key"), "wrong number of arguments");
    ExpectError(Execute(db, "TTL key extra"), "wrong number of arguments");
    ExpectError(Execute(db, "EXPIRE key nope"), "invalid expire time");
}

TEST(CommandHandler, InvalidListArgumentsAreReportedByCommand) {
    DataBase db;

    ExpectError(Execute(db, "LINDEX list nope"), "invalid index");
    ExpectError(Execute(db, "LSET list nope value"), "invalid index");
    ExpectError(Execute(db, "LPOP list nope"), "invalid count");
    ExpectError(Execute(db, "RPOP list nope"), "invalid count");
    ExpectError(Execute(db, "LINSERT list MIDDLE pivot value"), "invalid insert position");
    ExpectError(Execute(db, "LRANGE list 0 nope"), "invalid index");
}

TEST(CommandHandler, GeoCommandsAreRegistered) {
    DataBase db;

    ExpectInteger(
        Execute(db, "GEOADD Sicily 13.361389 38.115556 Palermo 15.087269 37.502669 Catania"),
        2
    );
    ExpectDistanceMetersNear(
        Execute(db, "GEODIST Sicily Palermo Catania km"),
        166274.1516,
        10.0,
        1000.0
    );
}

TEST(CommandHandler, GeoSearchParserAcceptsRequiredSyntax) {
    DataBase db;

    ExpectArray(
        Execute(db, "GEOSEARCH Sicily FROMLONLAT 15 37 BYRADIUS 100 km"),
        {}
    );
}

TEST(CommandHandler, GeoSearchParserAcceptsOptionalSortAndCountInAnyOrder) {
    DataBase db;

    ExpectArray(
        Execute(db, "GEOSEARCH Sicily FROMLONLAT 15 37 BYRADIUS 100 km ASC COUNT 2"),
        {}
    );
    ExpectArray(
        Execute(db, "GEOSEARCH Sicily FROMLONLAT 15 37 BYRADIUS 100 km COUNT 2 DESC"),
        {}
    );
}

TEST(CommandHandler, GeoSearchParserRejectsMalformedSyntax) {
    DataBase db;

    ExpectError(
        Execute(db, "GEOSEARCH Sicily FROMMEMBER Palermo BYRADIUS 100 km"),
        "wrong number of arguments"
    );
    ExpectError(
        Execute(db, "GEOSEARCH Sicily FROMLONLAT 15 37 BYBOX 100 km"),
        "syntax error"
    );
    ExpectError(
        Execute(db, "GEOSEARCH Sicily FROMLONLAT 15 37 BYRADIUS 100 km ASC DESC"),
        "syntax error"
    );
    ExpectError(
        Execute(db, "GEOSEARCH Sicily FROMLONLAT 15 37 BYRADIUS 100 km COUNT"),
        "syntax error"
    );
}

TEST(CommandHandler, GeoSearchParserRejectsInvalidNumbersAndUnits) {
    DataBase db;

    ExpectError(
        Execute(db, "GEOSEARCH Sicily FROMLONLAT bad 37 BYRADIUS 100 km"),
        "invalid longitude or latitude"
    );
    ExpectError(
        Execute(db, "GEOSEARCH Sicily FROMLONLAT 15 90 BYRADIUS 100 km"),
        "invalid longitude or latitude"
    );
    ExpectError(
        Execute(db, "GEOSEARCH Sicily FROMLONLAT 15 37 BYRADIUS -1 km"),
        "invalid radius"
    );
    ExpectError(
        Execute(db, "GEOSEARCH Sicily FROMLONLAT 15 37 BYRADIUS 100 yard"),
        "unsupported unit"
    );
    ExpectError(
        Execute(db, "GEOSEARCH Sicily FROMLONLAT 15 37 BYRADIUS 100 km COUNT zero"),
        "invalid count"
    );
    ExpectError(
        Execute(db, "GEOSEARCH Sicily FROMLONLAT 15 37 BYRADIUS 100 km COUNT 0"),
        "invalid count"
    );
}

TEST(CommandHandler, MemoryUsageReportsWrongArgumentCount) {
    DataBase db;

    ExpectError(Execute(db, "MEMORY USAGE"), "wrong number of arguments");
    ExpectError(Execute(db, "MEMORY USAGE a b"), "wrong number of arguments");
}

TEST(CommandHandler, ConfigCommandsReportWrongArgumentCount) {
    DataBase db;

    ExpectError(Execute(db, "CONFIG SET maxmemory"), "wrong number of arguments");
    ExpectError(Execute(db, "CONFIG SET maxmemory 1 2"), "wrong number of arguments");
    ExpectError(Execute(db, "CONFIG GET"), "wrong number of arguments");
    ExpectError(Execute(db, "CONFIG GET maxmemory extra"), "wrong number of arguments");
}

TEST(CommandHandler, ConfigCommandsRejectUnsupportedParameter) {
    DataBase db;

    ExpectError(Execute(db, "CONFIG SET timeout 10"), "unsupported config parameter");
    ExpectError(Execute(db, "CONFIG GET timeout"), "unsupported config parameter");
}

TEST(CommandHandler, ConfigSetParsesMemorySuffixes) {
    DataBase db;

    ExpectDone(Execute(db, "CONFIG SET maxmemory 5b"));
    ExpectInteger(Execute(db, "CONFIG GET maxmemory"), 5);
    ExpectDone(Execute(db, "CONFIG SET maxmemory 2kb"));
    ExpectInteger(Execute(db, "CONFIG GET maxmemory"), 2048);
    ExpectDone(Execute(db, "CONFIG SET maxmemory 3mb"));
    ExpectInteger(Execute(db, "CONFIG GET maxmemory"), 3 * 1024 * 1024);
}

TEST(CommandHandler, ConfigSetRejectsInvalidMemorySizes) {
    DataBase db;

    ExpectError(Execute(db, "CONFIG SET maxmemory nope"), "invalid maxmemory value");
    ExpectError(Execute(db, "CONFIG SET maxmemory -1"), "invalid maxmemory value");
    ExpectError(Execute(db, "CONFIG SET maxmemory 1tb"), "invalid maxmemory value");
}

TEST(CommandHandler, ConfigSetRejectsOverflowingMemorySize) {
    DataBase db;

    ExpectError(
        Execute(db, "CONFIG SET maxmemory 9999999999999999999gb"),
        "invalid maxmemory value"
    );
}

TEST(CommandHandler, UnknownSecondWordOfMultiWordCommandIsUnknownCommand) {
    DataBase db;

    ExpectError(Execute(db, "CONFIG RESET maxmemory"), "unknown command");
    ExpectError(Execute(db, "MEMORY GET key"), "unknown command");
}

TEST(CommandHandler, MultiWordCommandsAreCaseInsensitive) {
    DataBase db;

    ExpectDone(Execute(db, "config set maxmemory 0"));
    ExpectDone(Execute(db, "set key value"));
    EXPECT_GT(GetInteger(Execute(db, "memory usage key")), 0);
}

TEST(CommandHandler, ListInsertPositionLowercaseAccepted) {
    DataBase db;

    ExpectInteger(Execute(db, "RPUSH list a c"), 2);
    ExpectInteger(Execute(db, "LINSERT list before c b"), 3);
    ExpectArray(Execute(db, "LRANGE list 0 -1"), {"a", "b", "c"});
}

TEST(CommandHandler, ListInsertPositionMixedCaseRejected) {
    DataBase db;

    ExpectError(Execute(db, "LINSERT list Before pivot value"), "invalid insert position");
}

TEST(CommandHandler, ListCountRejectsNegativeCount) {
    DataBase db;

    ExpectError(Execute(db, "LPOP list -1"), "invalid count");
    ExpectError(Execute(db, "RPOP list -1"), "invalid count");
}

TEST(CommandHandler, ListIndexRejectsPlusSign) {
    DataBase db;

    ExpectError(Execute(db, "LINDEX list +1"), "invalid index");
    ExpectError(Execute(db, "LRANGE list +0 1"), "invalid index");
}

TEST(CommandHandler, EmptyParsedCommandFromWhitespaceLineReturnsEmptyCommandError) {
    DataBase db;
    Parser parser;
    CommandHandler handler;

    ExpectError(handler.Execute(db, parser.Parse("   \t  ")), "empty command");
}

TEST(CommandHandler, OomErrorIsReturnedThroughCommandHandler) {
    DataBase db;

    ExpectDone(Execute(db, "CONFIG SET maxmemory 1"));
    ExpectError(
        Execute(db, "SET key value"),
        "OOM command not allowed when used memory > 'maxmemory'"
    );
    ExpectString(Execute(db, "TYPE key"), "none");
}

TEST(CommandArgumentEquivalenceClasses, KnownCommandsRejectMissingAndExtraArguments) {
    Parser parser;
    CommandHandler handler;
    DataBase db;
    const std::vector<std::string> invalid_commands = {
        "SET", "SET key", "GET", "GET key extra", "STRLEN", "APPEND key",
        "TYPE", "TYPE key extra", "KEYS", "TTL key extra", "EXPIRE key",
        "CONFIG SET maxmemory", "CONFIG GET", "LPUSH list", "LLEN list extra",
        "LINDEX list", "LSET list 0", "LPOP list 0 extra", "LINSERT list BEFORE p",
        "LRANGE list 0", "SADD set", "SMEMBERS set extra", "SISMEMBER set",
        "SMOVE a b", "GEOADD geo", "GEOPOS geo", "GEODIST geo a",
        "GEOSEARCH", "GEOSEARCHSTORE dest source"
    };
    for (const auto& line : invalid_commands) {
        SCOPED_TRACE(line);
        ExpectError(handler.Execute(db, parser.Parse(line)), "wrong number of arguments");
    }
}
