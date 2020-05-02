#pragma once 

#include "tessera_impl.h"
#include "tessera/tile.h"
#include "tessera/tile_patch.h"
#include "expr_value.h"
#include <vector>
#include <map>

namespace tess {

    class allocator;

    class tile_patch::impl_type : public tessera_impl
    {
    private:
        std::vector<tess::tile> tiles_;
        std::map<std::string, expr_value> fields_;
    public:
		impl_type(const std::vector<tess::tile>& tiles);
        const std::vector<tess::tile>& tiles() const;
		expr_value get_field(allocator& allocator, const std::string& field) const;
        expr_value get_ary_item(int i) const;
        int get_ary_count() const;
		void apply(const matrix& mat);
        bool is_untouched() const;
        void insert_field(const std::string& var, const expr_value& val);
    };

    class cluster::impl_type : public tessera_impl
    {
    private:
        std::vector<expr_value> values_;
    public:
        impl_type(const std::vector<expr_value>& tiles);
        expr_value get_field(allocator& allocator, const std::string& field) const;
        expr_value get_ary_item(int i) const;
        int get_ary_count() const;
        const std::vector<expr_value>& values();
        void insert_field(const std::string& var, const expr_value& val);
    };
}