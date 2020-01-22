#pragma once

#include <boost/spirit/home/x3.hpp>
#include "../expression.h"
#include <memory>
#include <string>

namespace tess {
    namespace parser {
        namespace x3 = boost::spirit::x3;
        using expr_type = x3::rule<class expr_, std::shared_ptr<expression>>;
        BOOST_SPIRIT_DECLARE(expr_type);
        const expr_type& expr_parser();

        using indentifier_parser_type = x3::rule<class indentifier_parser_, std::string>;
        BOOST_SPIRIT_DECLARE(indentifier_parser_type);
        const indentifier_parser_type& identifier_parser();
    }
}