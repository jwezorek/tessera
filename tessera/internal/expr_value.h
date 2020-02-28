#pragma once

#include "../include/tessera/error.h"
#include <variant>

namespace tess {

    class tile_val {

    };

    class patch_val {

    };

    class number_val {

    };

    class bool_val {
        bool val_;
    public:
        bool_val(bool v);
        bool value() const;
    };

    class edge_val {

    };

    class vertex_val {

    };

    class nil_val {
    public:
        nil_val();
    };

    using expr_value = std::variant< tile_val, patch_val, number_val, bool_val, edge_val, vertex_val, nil_val, error>;
}