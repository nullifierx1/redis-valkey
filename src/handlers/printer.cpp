#include <handlers/printer.hpp>
#include <types/result.hpp>

#include <iostream>
#include <cctype>
#include <string_view>
#include <cstddef>
#include <variant>
#include <cstdint>
#include <string>

bool Printer::IsExitCommand(std::string_view command) {
    if (command.size() != 4) return false;

    auto to_upper = [](char w){
        return std::toupper(static_cast<unsigned char>(w));
    };

    return to_upper(command[0]) == 'E' && 
           to_upper(command[1]) == 'X' && 
           to_upper(command[2]) == 'I' && 
           to_upper(command[3]) == 'T';
}

void Printer::PrintResult(const Result& result) {
    std::visit(
        Overload{
            [](const DoneResult&) {
                std::cout << "OK\n";
            },
            [](const NilResult&) {
                std::cout << "(nil)\n";
            },
            [](const ErrorResult& error) {
                std::cerr << "(error) " << error.message << '\n';
            },
            [](const SimpleStringResult& result) {
                std::cout << result.value << '\n';
            },
            [](int64_t value) {
                std::cout << "(integer) " << value << '\n';
            },
            [](const std::string& value) {
                std::cout << '"' << value << '"' << '\n';
            },
            [](const ArrayResult& array) {
                auto n = array.values.size();
                if (n == 0) std::cout << "(empty array)" << '\n';
                for (size_t i = 0; i < n; ++i) {
                    std::cout << i + 1 << ") \"" << array.values[i] << "\"\n";
                }
            },
            [](const GeoPositionResult& positions) {
                auto n = positions.values.size();
                if (n == 0) std::cout << "(empty array)" << '\n';
                for (size_t i = 0; i < n; ++i) {
                    std::cout << i + 1 << ") ";
                    if (!positions.values[i]) {
                        std::cout << "(nil)\n";
                        continue;
                    }

                    std::cout << "1) \"" << positions.values[i]->longitude << "\"\n";
                    std::cout << "   2) \"" << positions.values[i]->latitude << "\"\n";
                }
            }
        },
        result.Value()
    );
}

void Printer::PrintLineOrigin(Printer::LineOriginType type) {
    switch (type) {
        case LineOriginType::kRedisCli : 
            std::cout << "redis-cli> "; 
            break;

        case LineOriginType::kNothing :
            std::cout << "> ";
            break;

        case LineOriginType::kConnection :
            std::cout << "127.0.0.1:6379> ";
            break;
    }
}

void Printer::PrintLogo(Printer::LogoType type) {
    switch (type) {
        case LogoType::kNothing : 
            break;

        case LogoType::kMan : 
            std::cout << R"(              
                $$$$$$$^^~~~~~~~~~~~~~~~~~~~~^^$$$$$$$$+------------------------------------+
                $$$$$$^     ~^  $  $$ $ $ $ I  ~^$$$$$$|           -LABWORK 10-             |
                $$$$$            ~ ~~ ~I          $$$$$| -RedisLab-InMemoryDataStoreOnline- |
                $$$$'                  '  _,w$<    $$$$|   -TYPES: String, List, Set, Geo-  |
                $$$$     $$$$$$$$w___,w$$$$$$$$  $  $$$|------------------------------------|
                $$$$     $$$$$$$$$$$$$$$$$$$$$$  I  $$$| -Store keys in std::unordered_map- |
                $$$$     $$$$$$$$$$$$$$$$$$$$*$[ i  $$$|-Weekdays 9pm-4pm, Weekends 9pm-9am-|
                $$$$     $$$$$$$$$$$$$$$$$$$$[][ | ]$$$|------------------------------------|
                $$$$     ~_,,_ ~$$$$$$$~ ____~ $    $$$| -Dedicated to things free! Speech- |
                $$$$    _~ ,  ,  `$$$~  _  _`$ ]L  J$$$| -Thought, Religion, Slack, fnord?- |
                $$$$  , $$w$ww+   $$$ww``,,$w$ ][  $$$$|------------------------------------|
                $$$$,  $$$$www$$$ $$$$$$$ww$$$$$[  $$$$| -Also Known As: The Metaphysical-  |
                $$$$$_|| $$$$$$P' $$P$$$$$$$$$$$[|c$$$$|   -Workshop and AlternativLand-    |
                $$$$$$w| '$$P~  P]$$$-~, ~Y$$^'],$$$$$$|------------------------------------|
                $$$$$$$[   _        _J$$Tk     ]]$$$$$$|-uXu Distribution Site-Online Games-|
                $$$$$$$$,$ $$, c,,,,,,,y ,w$$[ ,$$$$$$$|-Online Text Library-Text File Libs-|
                $$$$$$$$$ i $w   ====--_$$$$$  $$$$$$$$|-Messages Bases From Metaphysics to-|
                $$$$$$$$$$`,P~ _ ~^^^^Y$$$$$  $$$$$$$$$|-Underground Conversations-CU/H/P/A-|
                $$$$^^=^$$^   ^' ,ww,w$$$$$ _$$$$$$$$$$|-Encryption Areas-2500+ Text Files--|
                $$$_xJ~ ~   ,    $$$$$$$P~_$$$$$$$$$$$$|-Informaton on The Net-No Ratios----|
                $$   $,   ,$$$,_____   _,J$$$$$$$$$$$$$|-Free Access-Full Access First Call-|
                $$L  `' ,$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$|-MUD OnLine-Illuminations on Request|
                $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$+------------------------------------+
            )" << std::endl;
            break;
    }
}