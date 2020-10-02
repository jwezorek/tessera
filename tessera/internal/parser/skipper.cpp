#include "skipper.h"

namespace tess {
    namespace parser {

        x3::rule<class block_comment_> const block_comment = "block_comment";
        const auto block_comment_def = x3::lexeme["/*" >> *(block_comment | x3::char_ - "*/") > "*/"];

        const auto line_comment = x3::lexeme["//" >> *(x3::char_ - x3::eol) >> x3::eol];
        const auto skip_parser = x3::space | line_comment | block_comment;

        BOOST_SPIRIT_DEFINE(block_comment)
    }
}

std::tuple<bool, std::string::const_iterator> tess::parser::skipper::parse_aux(const text_range& input) const
{
    auto old_iter = input.begin();
    auto iter = input.begin();
    bool success = x3::parse(iter, input.end(), tess::parser::skip_parser);
    if (success) {
        return { true, iter };
    } else {
        return { false, old_iter };
    }
}