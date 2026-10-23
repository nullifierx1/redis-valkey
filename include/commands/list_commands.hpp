#pragma once

#include <commands/command_interface.hpp>
#include <commands/detail/number_parser.hpp>
#include <database/database.hpp>

#include <optional>
#include <cctype>
#include <cstddef>
#include <string>
#include <span>
#include <string_view>
#include <cstdint>

namespace list_detail {

[[nodiscard]] inline std::optional<size_t> ParseCount(const std::string& text) {
    return NumberParser::ParseNonNegativeDecimal<size_t>(text);
}

[[nodiscard]] inline std::optional<int64_t> ParseIndex(const std::string& text) {
    return NumberParser::ParseSignedDecimal<int64_t>(text);
}

inline std::optional<bool> ParseInsertPosition(const std::string& text) {
    if (text == "BEFORE" || text == "before") {
        return true;
    }

    if (text == "AFTER" || text == "after") {
        return false;
    }

    return std::nullopt;
}

}  // namespace list_detail

class LPushCommand final : public ICommand {
public:
    std::string_view Name() const noexcept override {
        return "LPUSH";
    }

    Result Execute(
        DataBase& database,
        std::span<const std::string> args) const override
    {
        if (args.size() < 2) {
            return Result::Error("wrong number of arguments");
        }

        return database.Lpush(args[0], args.subspan(1));
    }
};

class RPushCommand final : public ICommand {
public:
    std::string_view Name() const noexcept override {
        return "RPUSH";
    }

    Result Execute(
        DataBase& database,
        std::span<const std::string> args) const override
    {
        if (args.size() < 2) {
            return Result::Error("wrong number of arguments");
        }

        return database.Rpush(args[0], args.subspan(1));
    }
};

class LLenCommand final : public ICommand {
public:
    std::string_view Name() const noexcept override {
        return "LLEN";
    }

    Result Execute(
        DataBase& database,
        std::span<const std::string> args) const override
    {
        if (args.size() != 1) {
            return Result::Error("wrong number of arguments");
        }

        return database.Llen(args[0]);
    }
};

class LIndexCommand final : public ICommand {
public:
    std::string_view Name() const noexcept override {
        return "LINDEX";
    }

    Result Execute(
        DataBase& database,
        std::span<const std::string> args) const override
    {
        if (args.size() != 2) {
            return Result::Error("wrong number of arguments");
        }

        auto index = list_detail::ParseIndex(args[1]);
        if (!index.has_value()) {
            return Result::Error("invalid index");
        }

        return database.Lindex(args[0], index.value());
    }
};

class LSetCommand final : public ICommand {
public:
    std::string_view Name() const noexcept override {
        return "LSET";
    }

    Result Execute(
        DataBase& database,
        std::span<const std::string> args) const override
    {
        if (args.size() != 3) {
            return Result::Error("wrong number of arguments");
        }

        auto index = list_detail::ParseIndex(args[1]);
        if (!index.has_value()) {
            return Result::Error("invalid index");
        }

        return database.Lset(args[0], index.value(), args[2]);
    }
};

class LPopCommand final : public ICommand {
public:
    std::string_view Name() const noexcept override {
        return "LPOP";
    }

    Result Execute(
        DataBase& database,
        std::span<const std::string> args) const override
    {
        if (args.size() != 1 && args.size() != 2) {
            return Result::Error("wrong number of arguments");
        }

        if (args.size() == 1) {
            return database.Lpop(args[0], std::nullopt);
        }

        auto count = list_detail::ParseCount(args[1]);
        if (!count.has_value()) {
            return Result::Error("invalid count");
        }

        return database.Lpop(args[0], count);
    }
};

class RPopCommand final : public ICommand {
public:
    std::string_view Name() const noexcept override {
        return "RPOP";
    }

    Result Execute(
        DataBase& database,
        std::span<const std::string> args) const override
    {
        if (args.size() != 1 && args.size() != 2) {
            return Result::Error("wrong number of arguments");
        }

        if (args.size() == 1) {
            return database.Rpop(args[0], std::nullopt);
        }

        auto count = list_detail::ParseCount(args[1]);
        if (!count.has_value()) {
            return Result::Error("invalid count");
        }

        return database.Rpop(args[0], count);
    }
};

class LInsertCommand final : public ICommand {
public:
    std::string_view Name() const noexcept override {
        return "LINSERT";
    }

    Result Execute(
        DataBase& database,
        std::span<const std::string> args) const override
    {
        if (args.size() != 4) {
            return Result::Error("wrong number of arguments");
        }

        auto insert_before = list_detail::ParseInsertPosition(args[1]);
        if (!insert_before.has_value()) {
            return Result::Error("invalid insert position");
        }

        return database.Linsert(
            args[0], insert_before.value(), 
            args[2], args[3]
        );
    }
};

class LRangeCommand final : public ICommand {
public:
    std::string_view Name() const noexcept override {
        return "LRANGE";
    }

    Result Execute(
        DataBase& database,
        std::span<const std::string> args) const override
    {
        if (args.size() != 3) {
            return Result::Error("wrong number of arguments");
        }

        auto start = list_detail::ParseIndex(args[1]);
        auto stop = list_detail::ParseIndex(args[2]);

        if (!start.has_value() || !stop.has_value()) {
            return Result::Error("invalid index");
        }

        return database.Lrange(args[0], start.value(), stop.value());
    }
};
