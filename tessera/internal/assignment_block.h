#pragma once

#include <vector>
#include <tuple>
#include <string>
#include "expression.h"
#include "eval_context.h"

namespace tess {

	using var_assignment = std::tuple<std::string, tess::expr_ptr>;

	class assignment_block 
	{
		public:
			assignment_block(const std::vector<var_assignment>& assignments);
			scope_frame eval(eval_context& ctxt);
		private:
			std::vector<var_assignment> assignments_;
	};


}