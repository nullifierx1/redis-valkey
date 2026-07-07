#pragma once

#include <types/result.hpp>

#include <span>
#include <string>
#include <string_view>

class DataBase; /* forward delcaration */

class ICommand {
public:
    virtual ~ICommand() = default;

    virtual std::string_view Name() const noexcept = 0;

    virtual Result Execute(
        DataBase& database,
        std::span<const std::string> args
    ) const = 0;
};