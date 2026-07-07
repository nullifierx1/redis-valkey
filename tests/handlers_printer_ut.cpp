#include <handlers/printer.hpp>
#include <types/result.hpp>

#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <vector>
#include <utility>

TEST(Printer, DetectsExitCommandCaseInsensitively) {
    Printer printer;

    EXPECT_TRUE(printer.IsExitCommand("EXIT"));
    EXPECT_TRUE(printer.IsExitCommand("exit"));
    EXPECT_TRUE(printer.IsExitCommand("ExIt"));
    EXPECT_FALSE(printer.IsExitCommand("exit now"));
    EXPECT_FALSE(printer.IsExitCommand("quit"));
    EXPECT_FALSE(printer.IsExitCommand(""));
    EXPECT_FALSE(printer.IsExitCommand(" ex"));
}

TEST(Printer, PrintsScalarResults) {
    Printer printer;

    testing::internal::CaptureStdout();
    printer.PrintResult(Result::Done());
    printer.PrintResult(Result::Nil());
    printer.PrintResult(Result::Integer(42));
    printer.PrintResult(Result::String("value"));
    const std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(output, "OK\n(nil)\n(integer) 42\n\"value\"\n");
}

TEST(Printer, PrintsSimpleStringResultsWithoutQuotes) {
    Printer printer;

    testing::internal::CaptureStdout();
    printer.PrintResult(Result::SimpleString("string"));
    const std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(output, "string\n");
}

TEST(Printer, PrintsErrorsToStderr) {
    Printer printer;

    testing::internal::CaptureStderr();
    printer.PrintResult(Result::Error("wrong type"));
    const std::string output = testing::internal::GetCapturedStderr();

    EXPECT_EQ(output, "(error) wrong type\n");
}

TEST(Printer, PrintsArraysAndEmptyArrays) {
    Printer printer;

    testing::internal::CaptureStdout();
    printer.PrintResult(Result::Array({"one", "two"}));
    printer.PrintResult(Result::Array({}));
    const std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(output, "1) \"one\"\n2) \"two\"\n(empty array)\n");
}

TEST(Printer, PrintsGeoPositionResults) {
    Printer printer;
    std::vector<std::optional<GeoPosition>> values;
    values.push_back(GeoPosition{.longitude = "13.361389", .latitude = "38.115556"});
    values.push_back(std::nullopt);

    testing::internal::CaptureStdout();
    printer.PrintResult(Result::GeoPositions(std::move(values)));
    const std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(output, "1) 1) \"13.361389\"\n   2) \"38.115556\"\n2) (nil)\n");
}

TEST(Printer, PrintsEmptyGeoPositionResultAsEmptyArray) {
    Printer printer;

    testing::internal::CaptureStdout();
    printer.PrintResult(Result::GeoPositions({}));
    const std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(output, "(empty array)\n");
}

TEST(Printer, PrintsPromptAndLogo) {
    Printer printer;

    testing::internal::CaptureStdout();
    printer.PrintLineOrigin(Printer::LineOriginType::kRedisCli);
    const std::string prompt = testing::internal::GetCapturedStdout();
    EXPECT_EQ(prompt, "redis-cli> ");

    testing::internal::CaptureStdout();
    printer.PrintLogo(Printer::LogoType::kMan);
    const std::string logo = testing::internal::GetCapturedStdout();
    EXPECT_NE(logo.find("LABWORK 10"), std::string::npos);
}

TEST(Printer, ExitCommandRequiresExactlyExitToken) {
    Printer printer;

    EXPECT_FALSE(printer.IsExitCommand(" EXIT"));
    EXPECT_FALSE(printer.IsExitCommand("EXIT "));
    EXPECT_FALSE(printer.IsExitCommand("EXITS"));
    EXPECT_FALSE(printer.IsExitCommand("EXI"));
}

TEST(Printer, PrintsNegativeInteger) {
    Printer printer;

    testing::internal::CaptureStdout();
    printer.PrintResult(Result::Integer(-2));
    const std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(output, "(integer) -2\n");
}

TEST(Printer, PrintsEmptyStringAsBlankLine) {
    Printer printer;

    testing::internal::CaptureStdout();
    printer.PrintResult(Result::String(""));
    const std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(output, "\"\"\n");
}

TEST(Printer, PrintsArrayValuesContainingSpacesAsSingleValues) {
    Printer printer;

    testing::internal::CaptureStdout();
    printer.PrintResult(Result::Array({"hello world", "x"}));
    const std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(output, "1) \"hello world\"\n2) \"x\"\n");
}

TEST(Printer, PrintsGeoPositionWithSingleValue) {
    Printer printer;
    std::vector<std::optional<GeoPosition>> values;
    values.push_back(GeoPosition{.longitude = "1", .latitude = "2"});

    testing::internal::CaptureStdout();
    printer.PrintResult(Result::GeoPositions(std::move(values)));
    const std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(output, "1) 1) \"1\"\n   2) \"2\"\n");
}
