#pragma once

#include <cctype>
#include <string>
#include <vector>

struct ParsedCommand {
    std::vector<std::string> tokens;
};

class Parser {
public:
    ParsedCommand Parse(const std::string& line) const;

private:
    static bool IsSpace(char ch) {
        return std::isspace(static_cast<unsigned char>(ch)) != 0;
    }
};