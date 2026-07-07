#pragma once

#include <commands/command_interface.hpp>
#include <database/database.hpp>
#include <handlers/parser.hpp>

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

class CommandHandler {
public:
    CommandHandler();
    Result Execute(
        DataBase& data_base, 
        const ParsedCommand& command
    ) const;

private:
    struct CommandLookup {
        const ICommand* command = nullptr;
        size_t args_start = 0;
    };

    void Add(std::unique_ptr<ICommand> command);

    CommandLookup FindCommand(
        const std::vector<std::string>& tokens
    ) const;
    
    static std::string ToUpper(std::string_view value);

    std::unordered_map<std::string, std::unique_ptr<ICommand>> commands_;
};