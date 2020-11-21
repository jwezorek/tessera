#pragma once

#include "value.h"
#include <vector>
#include <memory>

namespace tess {

    namespace detail {
        class cluster_impl;
    }

    class cluster {
            friend class tessera_impl;
        public:
            using impl_type = detail::cluster_impl;
            std::vector<value_> items() const;
            int count() const;
        private:
            const impl_type*  impl_;
    };

}