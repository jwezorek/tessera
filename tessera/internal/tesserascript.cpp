#include "tessera/tesserascript.h"
#include "expression.h"
#include <iostream>

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

tess::tessera_script::tessera_script(const std::shared_ptr<expression>& e) : test_(e)
{
}

double tess::tessera_script::evaluate(const std::unordered_map<std::string, double>& vars) const
{
    return test_->eval(
        tess::eval_ctxt{ vars }
    );
}
