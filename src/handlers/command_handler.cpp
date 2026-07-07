#include <handlers/command_handler.hpp>
#include <handlers/parser.hpp>
#include <commands/string_commands.hpp>
#include <commands/common_commands.hpp>
#include <commands/list_commands.hpp>
#include <commands/set_commands.hpp>
#include <commands/geo_commands.hpp>
#include <types/result.hpp>
#include <database/database.hpp>

#include <cctype>
#include <memory>
#include <utility>
#include <span>
#include <vector>
#include <string_view>

CommandHandler::CommandHandler() {
    /* string commands */
    Add(std::make_unique<SetCommand>());
    Add(std::make_unique<GetCommand>());
    Add(std::make_unique<StrLenCommand>());
    Add(std::make_unique<AppendCommand>());

    /* common commands */
    Add(std::make_unique<TypeCommand>());
    Add(std::make_unique<DelCommand>());
    Add(std::make_unique<ExistsCommand>());
    Add(std::make_unique<DbSizeCommand>());
    Add(std::make_unique<FlushDbCommand>());
    Add(std::make_unique<KeysCommand>());
    Add(std::make_unique<ExpireCommand>());
    Add(std::make_unique<TtlCommand>());
    Add(std::make_unique<MemoryUsageCommand>());
    Add(std::make_unique<ConfigSetCommand>());
    Add(std::make_unique<ConfigGetCommand>());

    /* list commands */
    Add(std::make_unique<LPushCommand>());
    Add(std::make_unique<RPushCommand>());
    Add(std::make_unique<LLenCommand>());
    Add(std::make_unique<LIndexCommand>());
    Add(std::make_unique<LSetCommand>());
    Add(std::make_unique<LPopCommand>());
    Add(std::make_unique<RPopCommand>());
    Add(std::make_unique<LInsertCommand>());
    Add(std::make_unique<LRangeCommand>());

    /* set commands */
    Add(std::make_unique<SAddCommand>());
    Add(std::make_unique<SMembersCommand>());
    Add(std::make_unique<SCardCommand>());
    Add(std::make_unique<SIsMemberCommand>());
    Add(std::make_unique<SRemCommand>());
    Add(std::make_unique<SMoveCommand>());
    Add(std::make_unique<SDiffCommand>());
    Add(std::make_unique<SUnionCommand>());
    Add(std::make_unique<SInterCommand>());

    /* geo commands */
    Add(std::make_unique<GeoAddCommand>());
    Add(std::make_unique<GeoPosCommand>());
    Add(std::make_unique<GeoDistCommand>());
    Add(std::make_unique<GeoSearchCommand>());
    Add(std::make_unique<GeoSearchStoreCommand>());
}

Result CommandHandler::Execute(
    DataBase& database,
    const ParsedCommand& command) const 
{
    if (command.tokens.empty()) {
        return Result::Error("empty command");
    }

    CommandLookup lookup = FindCommand(command.tokens);
    if (lookup.command == nullptr) {
        return Result::Error("unknown command");
    }

    std::span<const std::string> args(
        command.tokens.data() + lookup.args_start,
        command.tokens.size() - lookup.args_start
    );

    return lookup.command->Execute(database, args);
}

void CommandHandler::Add(std::unique_ptr<ICommand> command) {
    std::string command_name = ToUpper(command->Name());
    commands_[std::move(command_name)] = std::move(command);
}

CommandHandler::CommandLookup CommandHandler::FindCommand(
    const std::vector<std::string>& tokens) const 
{
    if (tokens.size() >= 2) {
        std::string two_word = ToUpper(tokens[0]) + " " + ToUpper(tokens[1]);
        auto target = commands_.find(two_word);
        if (target != commands_.end()) {
            return {target->second.get(), 2};
        }
    }

    std::string one_word = ToUpper(tokens[0]);
    auto target = commands_.find(one_word);
    if (target == commands_.end()) return {};

    return {target->second.get(), 1};
}

std::string CommandHandler::ToUpper(std::string_view value) {
    std::string result;
    result.reserve(value.size());

    for (char ch : value) {
        result.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
    }

    return result;
}
