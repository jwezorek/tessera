#pragma once

#include <symengine/expression.h>
#include <symengine/logic.h>

namespace tess {

	template<typename T>
	struct point {
		T x;
		T y;
	};

	using SymPoint = point<SymEngine::Expression>;

	template<typename T>
	bool eval_bool(T a) {
		const auto& bool_exp = *a;
		return SymEngine::eval_double(bool_exp) == 1.0;
	}

	bool equals(const SymEngine::Expression& a, const SymEngine::Expression& b);
	bool equals(const SymPoint& a, const SymPoint& b);

}