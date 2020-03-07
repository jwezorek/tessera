#pragma once
#include "../include/tessera/tile.h"
#include "../include/tessera/tile_patch.h"
#include "../include/tessera/error.h"
#include <variant>
#include <symengine/expression.h>

namespace tess {

    using number = SymEngine::Expression;

    class nil_val {
    public:
        nil_val();
    };

	using expr_val_var = std::variant< tile, tile_patch, number, bool, edge, vertex, nil_val, error>;
	class expr_value : public expr_val_var
	{
	public:
		bool is_object() const;
		expr_value get_ary_item(int index) const;
		expr_value get_field(const std::string& field) const;
		expr_value get_field(const std::string& field, int ary_item_index) const;
	};

	
}