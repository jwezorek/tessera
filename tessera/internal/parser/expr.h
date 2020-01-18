#pragma once

#include <boost/spirit/home/x3.hpp>
#include "../expression.h"
#include <memory>

namespace tess {
    namespace parser {
        namespace x3 = boost::spirit::x3;
        using expr_type = x3::rule<class expr_, std::shared_ptr<expression>>;
        BOOST_SPIRIT_DECLARE(expr_type);

        expr_type expr_parser();
    }
}