#pragma once

#include <boost/spirit/home/x3.hpp>

#define SKIPPER_DEF \
	(x3::lexeme["/*" >> *(x3::char_ - "*/") >> "*/" | \
	"//" >> *~x3::char_("\r\n") >> x3::eol] | \
	x3::blank)

namespace tess {
    namespace parser
    {
        namespace x3 = boost::spirit::x3;

        using iterator_type = std::string::const_iterator;
        using context_type = x3::phrase_parse_context<decltype(SKIPPER_DEF)>::type;
    }
}
