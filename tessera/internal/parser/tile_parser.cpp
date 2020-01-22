#pragma once

#include "tile_parser_def.h"
#include "config.h"

namespace tess {
    namespace parser
    {
        BOOST_SPIRIT_INSTANTIATE(tile_type, iterator_type, context_type);
    }
}