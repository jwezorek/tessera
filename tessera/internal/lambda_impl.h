#pragma once

#include "function_def.h"
#include "execution_state.h"

namespace tess {
    class lambda::impl_type {
        public:
            function_def func;
            lex_scope::frame closure;

            impl_type(const function_def& f, const lex_scope::frame& c);
    };
}