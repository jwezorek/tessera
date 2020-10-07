#pragma once

#include <vector>
#include <memory>

namespace tess {

    class expr_value;

    namespace detail {
        class cluster_impl;
    }

    class cluster {
            friend class tessera_impl;
        public:
            using impl_type = detail::cluster_impl;
            const std::vector<expr_value>& items() const;
            int count() const;
        private:
            impl_type*  impl_;
    };

}