#pragma once

#include <memory>
#include <string>
#include <variant>

namespace tess
{
    class expression;

    class tessera_script
    {
    private:
        std::shared_ptr<expression> test_;

    public:
        tessera_script(const std::shared_ptr<expression>& e);
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
}