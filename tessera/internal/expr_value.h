#pragma once

#include "../include/tessera/error.h"
#include <variant>
#include <symengine/expression.h>

namespace tess {

    class tile_val {

    };

    class patch_val {

    };

    class number_val {
		SymEngine::Expression val_;
	public:
		number_val(int num);
		number_val(const SymEngine::Expression& expr);
		SymEngine::Expression value() const;
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

	using expr_val_var = std::variant< tile_val, patch_val, number_val, bool_val, edge_val, vertex_val, nil_val, error>;
	class expr_value : public expr_val_var
	{
	public:
		bool is_object() const;
		expr_value get_ary_item(int index) const;
		expr_value get_field(const std::string& field) const;
		expr_value get_field(const std::string& field, int ary_item_index) const;
	};

	
}