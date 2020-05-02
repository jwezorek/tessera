#pragma once
#include "../include/tessera/tile.h"
#include "../include/tessera/tile_patch.h"
#include "../include/tessera/error.h"
#include "cluster.h"
#include "tessera_impl.h"
#include "lambda.h"
#include "math_util.h"
#include <variant>
#include <memory>

namespace tess {

	class allocator;

    class nil_val {
    public:
        nil_val();
    };

	using expr_val_var = std::variant<nil_val, tile, tile_patch, number, bool, edge, vertex, lambda, cluster, error>;

	class expr_value : public expr_val_var, public tessera_impl
	{
	public:
		bool is_object_like() const;
		bool is_array_like() const;
		bool is_valid() const;
		bool is_error() const;
		expr_value get_ary_item(int index) const;
		int get_ary_count() const;
		expr_value get_field(allocator& allocator, const std::string& field) const;
		expr_value call(const std::vector<expr_value>& args) const;
		void insert_field(const std::string& var, expr_value val) const;
	};
	
}