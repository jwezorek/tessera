#pragma once

#include "../include/tessera/error.h"
#include <variant>
#include <symengine/expression.h>

namespace tess {

    using number = SymEngine::Expression;

    class tile_val {

    };

    class patch_val {

    };

    class edge_val {

    };

    class vertex_val {

    };

    class nil_val {
    public:
        nil_val();
    };

	using expr_val_var = std::variant< tile_val, patch_val, number, bool, edge_val, vertex_val, nil_val, error>;
	class expr_value : public expr_val_var
	{
	public:
		bool is_object() const;
		expr_value get_ary_item(int index) const;
		expr_value get_field(const std::string& field) const;
		expr_value get_field(const std::string& field, int ary_item_index) const;
	};

	
}