#pragma once

#include <vector>
#include <memory>

namespace tess {

    class expr_value;

    class cluster {
            friend class tessera_impl;
        public:
            cluster(const std::vector<expr_value>& values);
            class impl_type;
        private:
            std::shared_ptr<impl_type>  impl_;
    };

}