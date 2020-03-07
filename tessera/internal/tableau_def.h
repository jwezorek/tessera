#pragma once

#include "text_range.h"
#include <vector>

namespace tess {

    class tableau_def
    {
    private:
        text_range code_;
    public:
        tableau_def( const text_range& code = {} );
    };
}