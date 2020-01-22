#pragma once

#ifdef _DEBUG 
#pragma init_seg(lib) 
#endif 

#include "expr_def.h"
#include "config.h"

namespace tess {
    namespace parser
    {
        BOOST_SPIRIT_INSTANTIATE(expr_type, iterator_type, context_type);
        BOOST_SPIRIT_INSTANTIATE(indentifier_parser_type, iterator_type, context_type);
    }
}