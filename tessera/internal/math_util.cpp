#include "math_util.h"
#include "symengine/expression.h"
#include "symengine/functions.h"

/*
se::map_basic_basic vars = { {se::Expression("a"),se::Expression(1)}, {se::Expression("b"),se::Expression(2)} };
se::Expression expr2("(a + b) / 2");
auto output = expr2.subs(vars);


std::cout << output << " == " << se::eval_double(output) << "\n";


se::Expression three("1+1+1");
se::Expression eight("2 ** 3");
se::Expression one_hundred("50+50");
se::Expression phi("(1 + sqrt(5)) / 2");

std::cout << eight << " > " << three << " => " <<
	((se::Gt(eight, three) == se::boolTrue) ? "true" : "false") << "\n";

auto foo = se::Gt(phi, one_hundred);
const auto& foo2 = *foo;
auto test = eval_bool_expr(se::Gt( one_hundred, phi ));
std::cout << test << "\n";
int aaa;
aaa=4;
*/

namespace se = SymEngine;

namespace {

	template<typename T>
	bool eval_bool_expr(T expr) {
		const auto& bool_exp = *expr;
		return se::eval_double(bool_exp) == 1.0;
	}

	tess::point operator-(const tess::point& a) {
		auto [x, y] = a;
		return { -x, -y };
	}

	tess::matrix rotation_matrix(const tess::number& cos_theta, const tess::number& sin_theta) {
		using n = tess::number;
		return se::DenseMatrix(3, 3, {
				cos_theta, -sin_theta, n(0),
				sin_theta, cos_theta,  n(0),
				n(0),      n(0),       n(1)
			} 
		);
	}

	tess::matrix translation_matrix(const tess::number& x, const tess::number& y) {
		using n = tess::number;
		return se::DenseMatrix(3, 3, {
				n(1), n(0), x,
				n(0), n(1), y,
				n(0), n(0), n(1)
			}
		);
	}

	tess::matrix scale_matrix(const tess::number& x_scale, const tess::number& y_scale) {
		using n = tess::number;
		return se::DenseMatrix(3, 3, {
				x_scale, n(0),    n(0),
				n(0),    y_scale, n(0),
				n(0),    n(0),    n(1)
			}
		);
	}

	tess::matrix scale_matrix(const se::Expression& scale) {
		return scale_matrix(scale, scale);
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
		return se::sqrt(x_diff * x_diff + y_diff * y_diff);
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
		matrix.mul_matrix(translate, matrix);
		matrix.mul_matrix(rotate, matrix);
		matrix.mul_matrix(scale, matrix);

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
		matrix.mul_matrix(scale, matrix);
		matrix.mul_matrix(rotate, matrix);
		matrix.mul_matrix(translate, matrix);

		return matrix;
	}

}

bool tess::equals(const number& a, number& b)
{
    return eval_bool_expr(se::Eq(a, b));
}

bool tess::equals(const point& a, const point& b)
{
	auto [a_x, a_y] = a;
	auto [b_x, b_y] = b;
    return equals(a_x, b_x) && equals(a_y, b_y);
}

bool tess::is_ge(const number& lhs, number& rhs)
{
	return eval_bool_expr(se::Ge(lhs, rhs));
}

bool tess::is_gt(const number& lhs, number& rhs)
{
	return  eval_bool_expr(se::Gt(lhs, rhs));
}

bool tess::is_le(const number& lhs, number& rhs)
{
	return  eval_bool_expr(se::Le(lhs, rhs));
}

bool tess::is_lt(const number& lhs, number& rhs)
{
	return  eval_bool_expr(se::Lt(lhs, rhs));
}

bool tess::is_ne(const number& lhs, number& rhs)
{
	return  eval_bool_expr(se::Ne(lhs, rhs));
}

int tess::to_int(const number& num)
{
	return static_cast<int>( se::eval_double(num) );
}

tess::point tess::apply_matrix(const matrix& mat, const point& pt)
{
	using num = tess::number;
	matrix vec(3, 1, { num(std::get<0>(pt)), num(std::get<1>(pt)), num(1) } );
	mat.mul_matrix(vec, vec);
	return { vec.get(0,0), vec.get(1,0) };
}

tess::matrix tess::line_seg_to_line_seg(const line_segment& src, const line_segment& dest)
{
	auto from = from_line_seg(src);
	auto to = to_line_seg(dest);

	auto matrix = identity_matrix();
	matrix.mul_matrix(to, matrix);
	matrix.mul_matrix(from, matrix);

	return matrix;	
}

tess::number tess::pow(number base, number ex) {
	return se::pow(base, ex);
}

tess::number tess::acos(number arg) {
	return se::acos(arg);
}

tess::number tess::asin(number arg) {
	return se::asin(arg);
}

tess::number tess::atan(number arg) {
	return se::atan(arg);
}

tess::number tess::cos(number arg) {
	return se::cos(arg);
}

tess::number tess::sin(number arg) {
	return se::sin(arg);
}

tess::number tess::sqrt(number arg) {
	return se::sqrt(arg);
}

tess::number tess::tan(number arg) {
	return se::tan(arg);
}

tess::number tess::pi()
{
	return number(se::pi);
}
