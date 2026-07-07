#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

struct DoneResult {};

struct NilResult {};

struct ErrorResult {
    std::string message;
};

struct SimpleStringResult {
    std::string value;
};

struct ArrayResult {
    std::vector<std::string> values;
};

struct GeoPosition {
    std::string longitude;
    std::string latitude;
};

struct GeoPositionResult {
    std::vector<std::optional<GeoPosition>> values;
};

class Result {
public:
    using ResultValue = std::variant<
        DoneResult, NilResult,
        ErrorResult, SimpleStringResult, int64_t,
        std::string, ArrayResult,
        GeoPositionResult
    >;

    static Result Done() {
        return Result(DoneResult{});
    }

    static Result Nil() {
        return Result(NilResult{});
    }

    static Result Error(std::string message) {
        return Result(ErrorResult{std::move(message)});
    }

    static Result SimpleString(std::string value) {
        return Result(SimpleStringResult{std::move(value)});
    }

    static Result Integer(int64_t value) {
        return Result(value);
    }

    static Result String(std::string value) {
        return Result(std::move(value));
    }

    static Result Array(std::vector<std::string> values) {
        return Result(ArrayResult{std::move(values)});
    }

    static Result GeoPositions(std::vector<std::optional<GeoPosition>> values) {
        return Result(GeoPositionResult{std::move(values)});
    }

    const ResultValue& Value() const noexcept {
        return value_;
    }

private:
    explicit Result(ResultValue value)
        : value_(std::move(value))
    {}

    ResultValue value_;
};
