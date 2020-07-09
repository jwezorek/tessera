#pragma once

#include "number.h"
#include <memory>

namespace tess {
    
    class vertex_location_table {
    public:
        vertex_location_table();
        int get_index(const tess::point& pt) const;
        tess::point get_location(int index) const;
        int insert(const tess::point& pt);
        void apply_transformation(const matrix& mat);
    private:
        class impl_type;
        std::shared_ptr<impl_type> impl_;
    };

}