#pragma once

#include <vector>
#include <memory>

namespace tess {

    class expr_value;

    class cluster {
            friend class tessera_impl;
        public:
            class impl_type;
            const std::vector<expr_value>& items() const;
            int count() const;
        private:
            impl_type*  impl_;
    };

    bool operator==(cluster l, cluster r);

}