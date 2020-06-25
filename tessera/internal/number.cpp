#include "number.h"
#include <boost/math/constants/constants.hpp>

namespace bm = boost::multiprecision;

namespace {

	using vec = Eigen::Matrix<tess::number, 3, 1>;

	tess::point operator-(const tess::point& a) {
		auto [x, y] = a;
		return { -x, -y };
	}

	tess::matrix rotation_matrix(const tess::number& cos_theta, const tess::number& sin_theta) {
		tess::matrix rotation;
		rotation <<
			cos_theta,	-sin_theta, 0,
			sin_theta,	cos_theta,	0,
			0,			0,			1;
		return rotation;
	}

	tess::matrix translation_matrix(const tess::number& x, const tess::number& y) {
		tess::matrix translation;
		translation <<
			1, 0, x,
			0, 1, y,
			0, 0, 1;
		return translation;
	}

	tess::matrix scale_matrix(const tess::number& x_scale, const tess::number& y_scale) {
		tess::matrix scale;
		scale <<
			x_scale, 0,        0,
			0,       y_scale,  0,
			0,       0,        1;
		return scale;
	}

	tess::matrix scale_matrix(const tess::number& scale) {
		tess::matrix mat;
		mat <<
			scale,	0,		0,
			0,		scale,	0,
			0,		0,		1;
		return mat;
	}

	tess::matrix identity_matrix() {
		return scale_matrix(tess::number(1));
	}

	tess::matrix translation_matrix(const tess::point& pt) {
		return translation_matrix(std::get<0>(pt), std::get<1>(pt));
	}

	tess::number distance(const tess::point& u, const tess::point& v)
	{
		auto [x1, y1] = u;
		auto [x2, y2] = v;
		auto x_diff = x2 - x1;
		auto y_diff = y2 - y1;
		return bm::sqrt(x_diff * x_diff + y_diff * y_diff);
	}

	tess::matrix to_line_seg(const tess::line_segment& line_segment)
	{
		auto [u, v] = line_segment;
		auto [x1, y1] = std::get<0>(line_segment);
		auto [x2, y2] = std::get<1>(line_segment);
		auto target_length = distance(u, v);
		auto cosine = (x2 - x1) / target_length;
		auto sine = (y2 - y1) / target_length;

		auto rotate = rotation_matrix(cosine, sine);
		auto scale = scale_matrix(target_length);
		auto translate = translation_matrix(u);

		auto matrix = identity_matrix();
		matrix *= translate;
		matrix *= rotate;
		matrix *= scale;

		return matrix;
	}

	tess::matrix from_line_seg(const tess::line_segment& line_segment)
	{
		using num = tess::number;

		auto [u, v] = line_segment;
		auto [x1, y1] = std::get<0>(line_segment);
		auto [x2, y2] = std::get<1>(line_segment);
		auto src_length = distance(u, v);
		auto cosine = (x2 - x1) / src_length;
		auto sine = (y2 - y1) / src_length;

		auto rotate = rotation_matrix(cosine, -sine);
		auto scale = scale_matrix(num(1) / num(src_length));
		auto translate = translation_matrix(-u);

		auto matrix = identity_matrix();
		matrix *= scale;
		matrix *= rotate;
		matrix *= translate;

		return matrix;
	}

}

bool tess::equals(const number& a, number& b)
{
	return bm::abs(a - b) < std::numeric_limits<tess::number>::epsilon();
}

bool tess::equals(const point& a, const point& b)
{
	auto [a_x, a_y] = a;
	auto [b_x, b_y] = b;
    return equals(a_x, b_x) && equals(a_y, b_y);
}

bool tess::is_ge(const number& lhs, number& rhs)
{
	return (equals(lhs, rhs) || lhs > rhs);
}

bool tess::is_gt(const number& lhs, number& rhs)
{
	return lhs > rhs;
}

bool tess::is_le(const number& lhs, number& rhs)
{
	return (equals(lhs, rhs) || lhs < rhs);
}

bool tess::is_lt(const number& lhs, number& rhs)
{
	return lhs < rhs;
}

bool tess::is_ne(const number& lhs, number& rhs)
{
	return !equals(lhs, rhs);
}

int tess::to_int(const number& num)
{
	return static_cast<int>( num );
}

tess::point tess::apply_matrix(const matrix& mat, const point& pt)
{
	using num = tess::number;
	vec v;
	v << std::get<0>(pt), std::get<1>(pt), num(1);
	v = mat * v;
	return { v[0], v[1] };
}

tess::matrix tess::line_seg_to_line_seg(const line_segment& src, const line_segment& dest)
{
	auto from = from_line_seg(src);
	auto to = to_line_seg(dest);

	auto matrix = identity_matrix();
	matrix *= to;
	matrix *= from;

	return matrix;	
}

tess::matrix tess::flip_matrix()
{
	return scale_matrix(1, -1);
}

tess::number tess::pow(number base, number ex) {
	return bm::pow(base, ex);
}

tess::number tess::acos(number arg) {
	return bm::acos(arg);
}

tess::number tess::asin(number arg) {
	return bm::asin(arg);
}

tess::number tess::atan(number arg) {
	return bm::atan(arg);
}

tess::number tess::cos(number arg) {
	return bm::cos(arg);
}

tess::number tess::sin(number arg) {
	return bm::sin(arg);
}

tess::number tess::sqrt(number arg) {
	return bm::sqrt(arg);
}

tess::number tess::tan(number arg) {
	return bm::tan(arg);
}

tess::number tess::pi()
{
	return boost::math::constants::pi<tess::number>();
}

tess::number tess::phi()
{
	return boost::math::constants::phi<tess::number>();
}

tess::number tess::root_2()
{
	return boost::math::constants::root_two<tess::number>();
}

