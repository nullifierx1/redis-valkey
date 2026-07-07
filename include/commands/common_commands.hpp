#pragma once

#include <commands/command_interface.hpp>
#include <database/database.hpp>

#include <limits>

namespace common_detail {

inline std::optional<int64_t> ParseSeconds(const std::string& text) {
    if (text.empty()) return std::nullopt;

    int64_t value = 0;
    for (char ch : text) {
        if (!std::isdigit(static_cast<unsigned char>(ch))) {
            return std::nullopt;
        }

        value = value * 10 + static_cast<int64_t>(ch - '0');
    }

    return value;
}

inline std::optional<size_t> ParseMemorySize(const std::string& text) {
    if (text.empty()) return std::nullopt;

    size_t suffix_start = 0;
    while (suffix_start < text.size() &&
           std::isdigit(static_cast<unsigned char>(text[suffix_start]))) {
        ++suffix_start;
    }

    if (suffix_start == 0) return std::nullopt;

    size_t value = 0;
    for (size_t i = 0; i < suffix_start; ++i) {
        const size_t digit = static_cast<size_t>(text[i] - '0');
        if (value > (std::numeric_limits<size_t>::max() - digit) / 10) {
            return std::nullopt;
        }
        value = value * 10 + digit;
    }

    std::string suffix = text.substr(suffix_start);
    for (char& ch : suffix) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }

    size_t multiplier = 1;
    if (suffix.empty() || suffix == "b") {
        multiplier = 1;
    } else if (suffix == "kb") {
        multiplier = 1024;
    } else if (suffix == "mb") {
        multiplier = 1024 * 1024;
    } else if (suffix == "gb") {
        multiplier = 1024 * 1024 * 1024;
    } else {
        return std::nullopt;
    }

    if (value > std::numeric_limits<size_t>::max() / multiplier) {
        return std::nullopt;
    }

    return value * multiplier;
}

} // namespace common_detail

class ExistsCommand final : public ICommand {
public:
    std::string_view Name() const noexcept override {
        return "EXISTS";
    }

    Result Execute(
        DataBase& database,
        std::span<const std::string> args) const override
    {
        if (args.empty()) {
            return Result::Error("wrong number of arguments");
        }

        return database.Exists(args);
    }
};

class TypeCommand final : public ICommand {
public:
    std::string_view Name() const noexcept override {
        return "TYPE";
    }

    Result Execute(
        DataBase& database,
        std::span<const std::string> args) const override
    {
        if (args.size() != 1) {
            return Result::Error("wrong number of arguments");
        }

        return database.Type(args[0]);
    }
};

class DelCommand final : public ICommand {
public:
    std::string_view Name() const noexcept override {
        return "DEL";
    }

    Result Execute(
        DataBase& database,
        std::span<const std::string> args) const override
    {
        if (args.empty()) {
            return Result::Error("wrong number of arguments");
        }

        return database.Del(args);
    }
};

class DbSizeCommand final : public ICommand {
public:
    std::string_view Name() const noexcept override {
        return "DBSIZE";
    }

    Result Execute(
        DataBase& database,
        std::span<const std::string> args) const override
    {
        if (!args.empty()) {
            return Result::Error("wrong number of arguments");
        }

        return database.DbSize();
    }
};

class FlushDbCommand final : public ICommand {
public:
    std::string_view Name() const noexcept override {
        return "FLUSHDB";
    }

    Result Execute(
        DataBase& database,
        std::span<const std::string> args) const override
    {
        if (!args.empty()) {
            return Result::Error("wrong number of arguments");
        }

        return database.FlushDb();
    }
};

class KeysCommand final : public ICommand {
public:
    std::string_view Name() const noexcept override {
        return "KEYS";
    }

    Result Execute(
        DataBase& database,
        std::span<const std::string> args) const override
    {
        if (args.size() != 1) {
            return Result::Error("wrong number of arguments");
        }

        return database.Keys(args[0]);
    }
};

class ExpireCommand final : public ICommand {
public:
    std::string_view Name() const noexcept override {
        return "EXPIRE";
    }

    Result Execute(
        DataBase& database,
        std::span<const std::string> args) const override 
    {
        if (args.size() != 2) {
            return Result::Error("wrong number of arguments");
        }

        auto seconds = common_detail::ParseSeconds(args[1]);
        if (!seconds.has_value()) {
            return Result::Error("invalid expire time");
        }

        return database.Expire(args[0], seconds.value());
    }
};

class TtlCommand final : public ICommand {
public:
    std::string_view Name() const noexcept override {
        return "TTL";
    }

    Result Execute(
        DataBase& database,
        std::span<const std::string> args) const override 
    {
        if (args.size() != 1) {
            return Result::Error("wrong number of arguments");
        }

        return database.Ttl(args[0]);
    }
};

class MemoryUsageCommand final : public ICommand {
public:
    std::string_view Name() const noexcept override {
        return "MEMORY USAGE";
    }

    Result Execute(
        DataBase& database, 
        std::span<const std::string> args) const override 
    {
        if (args.size() != 1) {
            return Result::Error("wrong number of arguments");
        }

        return database.MemoryUsage(args[0]);
    }
};

class ConfigSetCommand final : public ICommand {
public:
    std::string_view Name() const noexcept override {
        return "CONFIG SET";
    }

    Result Execute(
        DataBase& database,
        std::span<const std::string> args) const override
    {
        if (args.size() != 2) {
            return Result::Error("wrong number of arguments");
        }

        if (args[0] != "maxmemory") {
            return Result::Error("unsupported config parameter");
        }

        auto bytes = common_detail::ParseMemorySize(args[1]);
        if (!bytes.has_value()) {
            return Result::Error("invalid maxmemory value");
        }

        return database.ConfigSetMaxMemory(bytes.value());
    }
};

class ConfigGetCommand final : public ICommand {
public:
    std::string_view Name() const noexcept override {
        return "CONFIG GET";
    }

    Result Execute(
        DataBase& database,
        std::span<const std::string> args) const override
    {
        if (args.size() != 1) {
            return Result::Error("wrong number of arguments");
        }

        if (args[0] != "maxmemory") {
            return Result::Error("unsupported config parameter");
        }

        return database.ConfigGetMaxMemory();
    }
};