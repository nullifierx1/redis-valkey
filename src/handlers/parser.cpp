#include <handlers/parser.hpp>

#include <string>
#include <cstddef>

ParsedCommand Parser::Parse(const std::string& line) const {
    ParsedCommand command;
    size_t start = 0;

    while (start < line.size()) {
        while (start < line.size() && IsSpace(line[start])) ++start;
        
        size_t end = start;
        while (end < line.size() && !IsSpace(line[end])) ++end;
        
        if (start < end) {
            command.tokens.push_back(line.substr(start, end - start));
        }
        start = end;
    }
    return command;
}