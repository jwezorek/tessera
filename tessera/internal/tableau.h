#pragma once

#include "text_range.h"
#include <vector>

namespace tess {

    class tableau
    {
    private:
        text_range code_;
    public:
        tableau( const text_range& code = {} );
    };

}