#pragma once

#include "../tile_def.h"
#include "../text_range.h"
#include "exception.h"
#include "../function.h"
#include <unordered_map>
#include <tuple>
#include <variant>
#include "util.h"

namespace tess {
    namespace parser {

        std::variant<tile_verts_and_edges, exception> parse_tile(const text_range& input);

        struct tile_def_ : public tess_expr<tile_def_> {
            std::tuple<tess::expr_ptr, std::string::const_iterator> parse_aux(const text_range& input) const;
        };
    }
}

