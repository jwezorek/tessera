#pragma once

#include <boost/multiprecision/cpp_bin_float.hpp>
#include <boost/multiprecision/eigen.hpp>
#include <Eigen/Dense>
#include <tuple>

namespace tess {

	using number = boost::multiprecision::cpp_bin_float_quad;
	using matrix = Eigen::Matrix<number, 3, 3>;
	using point = std::tuple<number,number>;
	using line_segment = std::tuple<point, point>;

	constexpr auto eps = std::numeric_limits<float>::epsilon() ;

	number pow(number base, number ex);
	number acos(number arg);
	number asin(number arg);
	number atan(number arg);
	number cos(number arg);
	number sin(number arg);
	number sqrt(number arg);
	number tan(number arg);
	number pi();
	number phi();
	number root_2();
	number abs(number arg);

	bool equals(const number& a, number& b);
	bool equals(const point& a, const point& b);

	bool is_ge(const number& lhs, number& rhs);
	bool is_gt(const number& lhs, number& rhs);
	bool is_le(const number& lhs, number& rhs);
	bool is_lt(const number& lhs, number& rhs);
	bool is_ne(const number& lhs, number& rhs);

	int to_int(const number& num);

	number distance(point p1, point p2);
	point apply_matrix(const matrix& matrix, const point& pt);
	matrix line_seg_to_line_seg(const line_segment& src, const line_segment& dest);
	matrix flip_matrix();
}