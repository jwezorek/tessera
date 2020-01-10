#pragma once

#include <string>
#include <variant>

namespace tess
{
    class tessera_script
    {
    public:
        tessera_script();
    };

    class parse_error
    {
    private:
        int line_;
        std::string msg_;
    public:
        parse_error(const std::string& msg, int line);
        const std::string& msg() const;
        int line() const;
    };

    std::variant<tessera_script, parse_error> parse(const std::string& script);
}