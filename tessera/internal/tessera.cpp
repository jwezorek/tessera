#include "../include/tessera/tessera.h"
#include "parser/parser.h"
#include <memory>

#include <symengine/expression.h>
#include <iostream>
#include <symengine/matrix.h>
#include <symengine/add.h>
#include <symengine/mul.h>
#include <symengine/pow.h>
#include <symengine/symengine_exception.h>
#include <symengine/visitor.h>
#include <tuple>

namespace se = SymEngine;

using Expr = se::Expression;
using sym_point = std::tuple<se::Expression, se::Expression>;
using sym_line_seg = std::tuple<sym_point, sym_point>;

sym_point operator-(const sym_point& a) {
    auto [x, y] = a;
    return { -x, -y };
}

se::DenseMatrix rotation_matrix(const Expr& cos_theta, const Expr& sin_theta) {
    return se::DenseMatrix(3, 3, {
            cos_theta, -sin_theta, Expr(0),
            sin_theta,  cos_theta, Expr(0),
            Expr(0),    Expr(0),   Expr(1)
        }
    );
}

se::DenseMatrix translation_matrix(const se::Expression& x, const se::Expression& y) {
    return se::DenseMatrix(3, 3, {
            Expr(1), Expr(0),      x,
            Expr(0), Expr(1),      y,
            Expr(0), Expr(0), Expr(1)
        }
    );
}

se::DenseMatrix scale_matrix(const se::Expression& x_scale, const se::Expression& y_scale) {
    return se::DenseMatrix(3, 3, {
            x_scale, Expr(0), Expr(0),
            Expr(0), y_scale, Expr(0),
            Expr(0), Expr(0), Expr(1)
        }
    );
}

se::DenseMatrix scale_matrix(const se::Expression& scale) {
    return scale_matrix(scale, scale);
}

se::DenseMatrix identity_matrix() {
    return scale_matrix(Expr(1));
}

se::DenseMatrix translation_matrix(const sym_point& pt) {
    return translation_matrix(std::get<0>(pt), std::get<1>(pt));
}

Expr distance(const sym_point& u, const sym_point& v)
{
    auto [x1, y1] = u;
    auto [x2, y2] = v;
    auto x_diff = x2 - x1;
    auto y_diff = y2 - y1;
    return se::sqrt(x_diff * x_diff + y_diff * y_diff);
}

se::DenseMatrix to_line_seg(const sym_line_seg& line_segment)
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

se::DenseMatrix from_line_seg(const sym_line_seg& line_segment)
{
    auto [u, v] = line_segment;
    auto [x1, y1] = std::get<0>(line_segment);
    auto [x2, y2] = std::get<1>(line_segment);
    auto src_length = distance(u, v);
    auto cosine = (x2 - x1) / src_length;
    auto sine = (y2 - y1) / src_length;

    auto rotate = rotation_matrix(cosine, -sine);
    auto scale = scale_matrix(Expr(1) / Expr(src_length));
    auto translate = translation_matrix(-u);

    auto matrix = identity_matrix();
    matrix.mul_matrix(scale, matrix);
    matrix.mul_matrix(rotate, matrix);
    matrix.mul_matrix(translate, matrix);

    return matrix;
}

se::DenseMatrix line_seg_to_line_seg(const sym_line_seg& src, const sym_line_seg& dest)
{
    auto from = from_line_seg(src);
    auto to = to_line_seg(dest);

    auto matrix = identity_matrix();
    matrix.mul_matrix(to, matrix);
    matrix.mul_matrix(from, matrix);

    return matrix;
}

sym_point apply_matrix(const se::DenseMatrix& matrix, const sym_point& pt) {
    se::DenseMatrix vec(3, 1, {
        se::Expression(std::get<0>(pt)),
        se::Expression(std::get<1>(pt)),
        se::Expression(1)
        }
    );
    matrix.mul_matrix(vec, vec);
    return { vec.get(0,0), vec.get(1,0) };
}


namespace {

	tess::error make_error(tess::text_range script, tess::parser::exception e)
	{
		int line_number = (e.has_where()) ?
			tess::text_range(script.begin(), e.where()).get_line_count() :
			-1;
		return tess::error(
			e.to_string(),
			line_number
		);
	}
}

template<typename T>
bool eval_bool_expr(T expr) {
    const auto& bool_exp = *expr;
    return se::eval_double(bool_exp) == 1.0;
}

void test_sym_engine()
{

    auto to_line_seg_matrix = to_line_seg({ {2, 2},{4,4} });

    auto p1 = apply_matrix(to_line_seg_matrix, { Expr(0), Expr(0) });
    auto p2 = apply_matrix(to_line_seg_matrix, { Expr(1) / Expr(2), Expr(0) });
    auto p3 = apply_matrix(to_line_seg_matrix, { Expr(1), Expr(0) });

    std::cout << "{ " << std::get<0>(p1) << " , " << std::get<1>(p1) << " }\n";
    std::cout << "{ " << std::get<0>(p2) << " , " << std::get<1>(p2) << " }\n";
    std::cout << "{ " << std::get<0>(p3) << " , " << std::get<1>(p3) << " }\n";

    std::cout << "---\n";

    auto from_line_seg_matrix = from_line_seg({ {-1,1}, {-3,4} });

    auto u1 = apply_matrix(from_line_seg_matrix, { Expr(-1), Expr(1) });
    auto u2 = apply_matrix(from_line_seg_matrix, { Expr(-2) , Expr(2) + Expr(1) / Expr(2) });
    auto u3 = apply_matrix(from_line_seg_matrix, { Expr(-3), Expr(4) });

    std::cout << "{ " << std::get<0>(u1) << " , " << std::get<1>(u1) << " }\n";
    std::cout << "{ " << std::get<0>(u2) << " , " << std::get<1>(u2) << " }\n";
    std::cout << "{ " << std::get<0>(u3) << " , " << std::get<1>(u3) << " }\n";

    std::cout << "---\n";

    auto l2l = line_seg_to_line_seg({ {-1,1}, {-3,4} }, { {2, 2},{4,4} });

    auto v1 = apply_matrix(l2l, { Expr(-1), Expr(1) });
    auto v2 = apply_matrix(l2l, { Expr(-2) , Expr(2) + Expr(1) / Expr(2) });
    auto v3 = apply_matrix(l2l, { Expr(-3), Expr(4) });

    std::cout << "{ " << std::get<0>(v1) << " , " << std::get<1>(v1) << " }\n";
    std::cout << "{ " << std::get<0>(v2) << " , " << std::get<1>(v2) << " }\n";
    std::cout << "{ " << std::get<0>(v3) << " , " << std::get<1>(v3) << " }\n";

    std::cout << "---\n";

    auto l2l_formula = line_seg_to_line_seg({ {Expr("a1_x"),Expr("a1_y")}, {Expr("a2_x"),Expr("a2_y")} }, { {Expr("b1_x"),Expr("b1_y")}, {Expr("b2_x"),Expr("b2_y")} });
    std::cout << l2l_formula << "\n";
    std::cout << "---\n";

    se::map_basic_basic vars = { {se::Expression("a"),se::Expression(1)}, {se::Expression("b"),se::Expression(2)} };
    se::Expression expr2("(a + b) / 2");
    auto output = expr2.subs(vars);


    std::cout << output << " == " << se::eval_double(output) << "\n";

    /*
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
}

std::variant<tess::tessera_script, tess::error> tess::parse(const std::string& script)
{
    //test_sym_engine();

	text_range source_code{ script };
    auto result = tess::parser::parse(source_code);
	if (std::holds_alternative<tess::tessera_script>(result))
		return std::get<tess::tessera_script>(result);
	else
		return make_error(source_code, std::get<tess::parser::exception>(result));
}
