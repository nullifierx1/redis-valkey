#include <handlers/parser.hpp>

#include <gtest/gtest.h>

#include <vector>
#include <string>

TEST(Parser, EmptyAndWhitespaceOnlyInputProduceNoTokens) {
    Parser parser;

    EXPECT_TRUE(parser.Parse("").tokens.empty());
    EXPECT_TRUE(parser.Parse(" \t\r\n ").tokens.empty());
}

TEST(Parser, SplitsCommandByAsciiWhitespace) {
    Parser parser;

    EXPECT_EQ(
        parser.Parse("  SET\tkey\r\nvalue  ").tokens,
        (std::vector<std::string>{"SET", "key", "value"})
    );
}

TEST(Parser, HandlesSingleTokenWithoutTrailingWhitespace) {
    Parser parser;

    EXPECT_EQ(
        parser.Parse("DBSIZE").tokens,
        (std::vector<std::string>{"DBSIZE"})
    );
}

TEST(Parser, TreatsFormFeedAndVerticalTabAsWhitespace) {
    Parser parser;

    EXPECT_EQ(
        parser.Parse("SET\fkey\vvalue").tokens,
        (std::vector<std::string>{"SET", "key", "value"})
    );
}

TEST(Parser, CollapsesManySeparatorsBetweenTokens) {
    Parser parser;

    EXPECT_EQ(
        parser.Parse("\t\tLPUSH    list      one\t\ttwo").tokens,
        (std::vector<std::string>{"LPUSH", "list", "one", "two"})
    );
}

TEST(Parser, KeepsQuotesAsPartOfTokenBecauseQuotesAreNotImplemented) {
    Parser parser;

    EXPECT_EQ(
        parser.Parse("GEOADD Sicily 13 38 \"Palermo\"").tokens,
        (std::vector<std::string>{"GEOADD", "Sicily", "13", "38", "\"Palermo\""})
    );
}

TEST(Parser, SplitsMultiWordCommandsIntoSeparateTokens) {
    Parser parser;

    EXPECT_EQ(
        parser.Parse("CONFIG SET maxmemory 64mb").tokens,
        (std::vector<std::string>{"CONFIG", "SET", "maxmemory", "64mb"})
    );
    EXPECT_EQ(
        parser.Parse("MEMORY USAGE key").tokens,
        (std::vector<std::string>{"MEMORY", "USAGE", "key"})
    );
}

TEST(Parser, KeepsPunctuationInsideTokens) {
    Parser parser;

    EXPECT_EQ(
        parser.Parse("SET key:value file.name").tokens,
        (std::vector<std::string>{"SET", "key:value", "file.name"})
    );
}

TEST(Parser, HandlesWindowsLineEndingAtEnd) {
    Parser parser;

    EXPECT_EQ(
        parser.Parse("GET key\r\n").tokens,
        (std::vector<std::string>{"GET", "key"})
    );
}

TEST(Parser, HandlesLongCommandLine) {
    Parser parser;

    EXPECT_EQ(
        parser.Parse("SADD set a b c d e f").tokens,
        (std::vector<std::string>{"SADD", "set", "a", "b", "c", "d", "e", "f"})
    );
}

TEST(Parser, DoesNotTreatCommaAsSeparator) {
    Parser parser;

    EXPECT_EQ(
        parser.Parse("SET key a,b,c").tokens,
        (std::vector<std::string>{"SET", "key", "a,b,c"})
    );
}
