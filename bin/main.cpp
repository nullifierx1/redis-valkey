#include <database/database.hpp>
#include <commands/common_commands.hpp>
#include <handlers/command_handler.hpp>
#include <handlers/parser.hpp>
#include <handlers/printer.hpp>
#include <types/result.hpp>

#include <iostream>
#include <string>
#include <optional>

bool ParseArgs(int argc, char** argv, DataBase& db) {
    if (argc == 3 && std::string(argv[1]) == "--maxmemory") {
        auto max_memory = common_detail::ParseMemorySize(argv[2]);
        if (!max_memory.has_value()) {
            std::cerr << "(error) invalid maxmemory value\n";
            return false;
        }

        db.SetMaxMemory(max_memory.value());
    } else if (argc != 1) {
        std::cerr << "(error) usage: valkey [--maxmemory <bytes>]\n";
        return false;
    }

    return true;
}

int main(int argc, char** argv) {
    std::ios::sync_with_stdio(false);
    std::cin.tie(0);

    Parser parser;
    CommandHandler handler;
    DataBase db;
    Printer printer;

    printer.PrintLogo(Printer::LogoType::kNothing);

    if (!ParseArgs(argc, argv, db)) return 1;

    std::string command;
    while (true) {
        if (!std::getline(std::cin, command)) break; // EOF 
        if (printer.IsExitCommand(command)) break;

        ParsedCommand parsed_command = parser.Parse(command);
        Result result = handler.Execute(db, parsed_command);

        printer.PrintResult(result);
    }

    return 0;
}