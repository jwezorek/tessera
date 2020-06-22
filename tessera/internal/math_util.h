#pragma once

#include <symengine/expression.h>
#include <symengine/logic.h>
#include <symengine/matrix.h>
#include <tuple>

namespace tess {

	using number = SymEngine::Expression;
	using matrix = SymEngine::DenseMatrix;
	using point = std::tuple<number,number>;
	using line_segment = std::tuple<point, point>;

	number pow(number base, number ex);
	number acos(number arg);
	number asin(number arg);
	number atan(number arg);
	number cos(number arg);
	number sin(number arg);
	number sqrt(number arg);
	number tan(number arg);
	number pi();

	bool equals(const number& a, number& b);
	bool equals(const point& a, const point& b);

	bool is_ge(const number& lhs, number& rhs);
	bool is_gt(const number& lhs, number& rhs);
	bool is_le(const number& lhs, number& rhs);
	bool is_lt(const number& lhs, number& rhs);
	bool is_ne(const number& lhs, number& rhs);

	int to_int(const number& num);

	point apply_matrix(const matrix& matrix, const point& pt);
	matrix line_seg_to_line_seg(const line_segment& src, const line_segment& dest);
	matrix flip_matrix();
}