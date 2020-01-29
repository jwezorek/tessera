#include "../include/tessera/tessera.h"
#include "parser/parser.h"
#include <memory>

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

std::variant<tess::tessera_script, tess::error> tess::parse(const std::string& script)
{
	text_range source_code{ script };
    auto result = tess::parser::parse(source_code);
	if (std::holds_alternative<tess::tessera_script>(result))
		return std::get<tess::tessera_script>(result);
	else
		return make_error(source_code, std::get<tess::parser::exception>(result));
}
