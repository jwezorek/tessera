#include "../include/tessera/script.h"
#include "script_impl.h"
#include "parser/script_parser.h"

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

std::variant<tess::script, tess::error> tess::script::interpret(const std::string& script)
{
    text_range source_code{ script };
    auto result = tess::parser::parse(source_code);
	if (std::holds_alternative<tess::script_specifier>(result)) {
		return tess::script(
			std::make_shared<impl_type>(
				std::make_from_tuple<impl_type>(std::get<tess::script_specifier>(result))
			)
		);
	} else {
		return make_error(source_code, std::get<tess::parser::exception>(result));
	}
}

tess::script::script(std::shared_ptr<impl_type> impl) : impl_(impl)
{}

const std::vector<std::string>& tess::script::parameters() const
{
    return {};
}

tess::result tess::script::execute(const std::vector<std::string>& args) const
{
    return result();
}
