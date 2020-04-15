#pragma once
#include "../include/tessera/tile.h"
#include "../include/tessera/tile_patch.h"
#include "../include/tessera/error.h"
#include "tessera_impl.h"
#include "math_util.h"
#include <variant>
#include <memory>

namespace tess {

    class nil_val {
    public:
        nil_val();
    };

	class function;

	using func_ptr = std::shared_ptr<const tess::function>;
	using expr_val_var = std::variant< nil_val, tile, tile_patch, number, bool, edge, vertex, func_ptr, error>;

	class expr_value : public expr_val_var, public tessera_impl
	{
	public:
		bool is_object() const;
		expr_value get_ary_item(int index) const;
		expr_value get_field(const std::string& field) const;
		expr_value get_field(const std::string& field, int ary_item_index) const;
	};
	
}