#pragma once

#include "text_range.h"
#include "statement.h"
#include <vector>
#include <string>

namespace tess {

    class tableau_def
    {
    private:
        std::vector<std::string> params_;
        stmts statements_;
    public:
        tableau_def() {}
        tableau_def(const std::vector<std::string>& params, const text_range& code);
        expr_value eval(const execution_ctxt&) const;
    };
}