#include "expr_def.h"
#include "config.h"

namespace tess {
    namespace parser
    {
        BOOST_SPIRIT_INSTANTIATE(expr_type, iterator_type, context_type);
    }
}