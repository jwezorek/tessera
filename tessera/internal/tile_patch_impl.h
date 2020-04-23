#pragma once 

#include "tessera_impl.h"
#include "tessera/tile.h"
#include "tessera/tile_patch.h"
#include "expr_value.h"
#include <vector>

namespace tess {

    class tile_patch::impl_type : public tessera_impl
    {
    private:
        std::vector<tess::tile> tiles_;
    public:
		impl_type(const std::vector<tess::tile>& tiles);
        const std::vector<tess::tile>& tiles() const;
		expr_value get_field(const std::string& field) const;
        expr_value get_ary_item(int i) const;
		void apply(const matrix& mat);
        bool is_untouched() const;
    };

    class cluster::impl_type : public tessera_impl
    {
    private:
        std::vector<expr_value> values_;
    public:
        impl_type(const std::vector<expr_value>& tiles);
        expr_value get_field(const std::string& field) const;
        expr_value get_ary_item(int i) const;
    };
}