#pragma once

#include "expr_value.h"

namespace tess {


    class execution_ctxt {

	public:

		expr_value call(const std::string& func, const std::vector<expr_value>& args) const;
		expr_value eval(const std::string& var) const;
    };

}