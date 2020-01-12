#include "tessera/tesserascript.h"
#include "lexer.h"
#include <iostream>

tess::tessera_script::tessera_script()
{
}

tess::parse_error::parse_error(const std::string& msg, int line) :
    msg_(msg), line_(line)
{
}

const std::string& tess::parse_error::msg() const
{
    return msg_;
}

int tess::parse_error::line() const
{
    return line_;
}

std::variant<tess::tessera_script, tess::parse_error> tess::parse(const std::string& script)
{
	lexer lex(script.begin(), script.end());
	for (auto i = lex.begin(); i != lex.end(); ++i)
		std::cout << *i << "\n";
    return tess::tessera_script();
}
