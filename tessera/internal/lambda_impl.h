#pragma once

#include "function_def.h"
#include "eval_context.h"

namespace tess {
    class lambda::impl_type {
        public:
            function_def func;
            scope_frame closure;

            impl_type(const function_def& f, const scope_frame& c);
    };
}