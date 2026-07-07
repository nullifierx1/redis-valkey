#pragma once

#include <types/result.hpp>

class Printer {
public:
    enum class LogoType {
        kNothing,
        kMan
    };

    enum class LineOriginType {
        kConnection,
        kRedisCli,
        kNothing
    };

    template <typename... Funcs>
    struct Overload : Funcs... {
        using Funcs::operator()...;
    };

    template <typename... Funcs>
    Overload(Funcs...) -> Overload<Funcs...>;

    bool IsExitCommand(std::string_view command);
    void PrintResult(const Result& result);
    
    void PrintLineOrigin(LineOriginType type);
    void PrintLogo(LogoType type);
};