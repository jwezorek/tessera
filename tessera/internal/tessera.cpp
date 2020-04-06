#include "../include/tessera/tessera.h"
#include "math_util.h"
#include "parser/parser.h"
#include <memory>
#include <iostream>
#include <tuple>

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



void test_sym_engine()
{
	using num = tess::number;
    auto l2l = tess::line_seg_to_line_seg({ {-1,1}, {-3,4} }, { {2, 2},{4,4} });

    auto v1 = tess::apply_matrix(l2l, { num(-1), num(1) });
    auto v2 = tess::apply_matrix(l2l, { num(-2) , num(2) + num(1) / num(2) });
    auto v3 = tess::apply_matrix(l2l, { num(-3), num(4) });

    std::cout << "{ " << std::get<0>(v1) << " , " << std::get<1>(v1) << " }\n";
    std::cout << "{ " << std::get<0>(v2) << " , " << std::get<1>(v2) << " }\n";
    std::cout << "{ " << std::get<0>(v3) << " , " << std::get<1>(v3) << " }\n";

    std::cout << "---\n";
}

std::variant<tess::tessera_script, tess::error> tess::parse(const std::string& script)
{
    test_sym_engine();

	text_range source_code{ script };
    auto result = tess::parser::parse(source_code);
	if (std::holds_alternative<tess::tessera_script>(result))
		return std::get<tess::tessera_script>(result);
	else
		return make_error(source_code, std::get<tess::parser::exception>(result));
}
