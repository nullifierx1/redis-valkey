#pragma once

#include <commands/command_interface.hpp>
#include <database/database.hpp>

#include <string>
#include <string_view>
#include <span>
#include <cctype>

class SetCommand final : public ICommand {
public:
    std::string_view Name() const noexcept override {
        return "SET";
    }

    Result Execute(
        DataBase& database,
        std::span<const std::string> args) const override 
    {
        if (args.size() != 2) {
            return Result::Error("wrong number of arguments");
        }

        return database.Set(args[0], args[1]);
    }
};

class GetCommand final : public ICommand {
public:
    std::string_view Name() const noexcept override {
        return "GET";
    }

    Result Execute(
        DataBase& database,
        std::span<const std::string> args) const override 
    {
        if (args.size() != 1) {
            return Result::Error("wrong number of arguments");
        }

        return database.Get(args[0]);
    }
};

class StrLenCommand final : public ICommand {
public:
    std::string_view Name() const noexcept override {
        return "STRLEN";
    }

    Result Execute(
        DataBase& database,
        std::span<const std::string> args) const override 
    {
        if (args.size() != 1) {
            return Result::Error("wrong number of arguments");
        }

        return database.Strlen(args[0]);
    }
};

class AppendCommand final : public ICommand {
public:
    std::string_view Name() const noexcept override {
        return "APPEND";
    }

    Result Execute(
        DataBase& database,
        std::span<const std::string> args) const override 
    {
        if (args.size() != 2) {
            return Result::Error("wrong number of arguments");
        }

        return database.Append(args[0], args[1]);
    }
};