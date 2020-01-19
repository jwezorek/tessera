#include "tessera/tesserascript.h"
#include "expression.h"
#include <iostream>

tess::tessera_script::tessera_script(const std::shared_ptr<expression>& e) : test_(e)
{
}

double tess::tessera_script::evaluate(const std::unordered_map<std::string, double>& vars) const
{
    return test_->eval(
        tess::eval_ctxt{ vars }
    );
}
