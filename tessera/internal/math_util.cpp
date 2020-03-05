#include "math_util.h"

namespace se = SymEngine;

bool tess::equals(const SymEngine::Expression& a, const SymEngine::Expression& b)
{
    return eval_bool(se::Eq(a, b));
}

bool tess::equals(const SymPoint& a, const SymPoint& b)
{
    return equals(a.x, b.x) && equals(a.y, b.y);
}
