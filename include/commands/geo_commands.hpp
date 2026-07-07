#pragma once

#include <commands/command_interface.hpp>
#include <database/database.hpp>

class GeoAddCommand final : public ICommand {
public:
    std::string_view Name() const noexcept override {
        return "GEOADD";
    }

    Result Execute(
        DataBase& database,
        std::span<const std::string> args) const override 
    {
        if (args.size() < 4 || args.size() % 3 != 1) {
            return Result::Error("wrong number of arguments");
        }

        return database.Geoadd(args[0], args.subspan(1));
    }
};

class GeoPosCommand final : public ICommand {
public:
    std::string_view Name() const noexcept override {
        return "GEOPOS";
    }

    Result Execute(
        DataBase& database,
        std::span<const std::string> args) const override 
    {
        if (args.size() < 2) {
            return Result::Error("wrong number of arguments");
        }

        return database.Geopos(args[0], args.subspan(1));
    }
};

class GeoDistCommand final : public ICommand {
public:
    std::string_view Name() const noexcept override {
        return "GEODIST";
    }

    Result Execute(
        DataBase& database,
        std::span<const std::string> args) const override 
    {
        if (args.size() != 3 && args.size() != 4) {
            return Result::Error("wrong number of arguments");
        }

        const std::string unit = args.size() == 4 ? args[3] : "m";
        return database.Geodist(args[0], args[1], args[2], unit);
    }
};

class GeoSearchCommand final : public ICommand {
public:
    std::string_view Name() const noexcept override {
        return "GEOSEARCH";
    }

    Result Execute(
        DataBase& database,
        std::span<const std::string> args) const override 
    {
        if (args.empty()) {
            return Result::Error("wrong number of arguments");
        }

        return database.Geosearch(args[0], args.subspan(1));
    }
};

class GeoSearchStoreCommand final : public ICommand {
public:
    std::string_view Name() const noexcept override {
        return "GEOSEARCHSTORE";
    }

    Result Execute(
        DataBase& database,
        std::span<const std::string> args) const override 
    {
        if (args.size() < 8) {
            return Result::Error("wrong number of arguments");
        }

        return database.Geosearchstore(args[0], args[1], args.subspan(2));
    }
};