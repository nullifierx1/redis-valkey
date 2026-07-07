#pragma once

#include <commands/command_interface.hpp>
#include <database/database.hpp>

class SAddCommand final : public ICommand {
public:
    std::string_view Name() const noexcept override {
        return "SADD";
    }

    Result Execute(
        DataBase& database,
        std::span<const std::string> args) const override 
    {
        if (args.size() < 2) {
            return Result::Error("wrong number of arguments");
        }

        return database.Sadd(args[0], args.subspan(1));
    }
};

class SMembersCommand final : public ICommand {
public:
    std::string_view Name() const noexcept override {
        return "SMEMBERS";
    }

    Result Execute(
        DataBase& database,
        std::span<const std::string> args) const override 
    {
        if (args.size() != 1) {
            return Result::Error("wrong number of arguments");
        }

        return database.Smembers(args[0]);
    }
};

class SCardCommand final : public ICommand {
public:
    std::string_view Name() const noexcept override {
        return "SCARD";
    }

    Result Execute(
        DataBase& database,
        std::span<const std::string> args) const override 
    {
        if (args.size() != 1) {
            return Result::Error("wrong number of arguments");
        }

        return database.Scard(args[0]);
    }
};

class SIsMemberCommand final : public ICommand {
public:
    std::string_view Name() const noexcept override {
        return "SISMEMBER";
    }

    Result Execute(
        DataBase& database,
        std::span<const std::string> args) const override 
    {
        if (args.size() != 2) {
            return Result::Error("wrong number of arguments");
        }

        return database.Sismember(args[0], args[1]);
    }
};

class SRemCommand final : public ICommand {
public:
    std::string_view Name() const noexcept override {
        return "SREM";
    }

    Result Execute(
        DataBase& database,
        std::span<const std::string> args) const override
    {
        if (args.size() < 2) {
            return Result::Error("wrong number of arguments");
        }

        return database.Srem(args[0], args.subspan(1));
    }
};

class SMoveCommand final : public ICommand {
public:
    std::string_view Name() const noexcept override {
        return "SMOVE";
    }

    Result Execute(
        DataBase& database,
        std::span<const std::string> args) const override
    {
        if (args.size() != 3) {
            return Result::Error("wrong number of arguments");
        }

        return database.Smove(args[0], args[1], args[2]);
    }
};

class SDiffCommand final : public ICommand {
public:
    std::string_view Name() const noexcept override {
        return "SDIFF";
    }

    Result Execute(
        DataBase& database,
        std::span<const std::string> args) const override
    {
        if (args.empty()) {
            return Result::Error("wrong number of arguments");
        }

        return database.Sdiff(args[0], args.subspan(1));
    }
};

class SUnionCommand final : public ICommand {
public:
    std::string_view Name() const noexcept override {
        return "SUNION";
    }

    Result Execute(
        DataBase& database,
        std::span<const std::string> args) const override
    {
        if (args.empty()) {
            return Result::Error("wrong number of arguments");
        }

        return database.Sunion(args);
    }
};

class SInterCommand final : public ICommand {
public:
    std::string_view Name() const noexcept override {
        return "SINTER";
    }

    Result Execute(
        DataBase& database,
        std::span<const std::string> args) const override
    {
        if (args.empty()) {
            return Result::Error("wrong number of arguments");
        }

        return database.Sinter(args[0], args.subspan(1));
    }
};
