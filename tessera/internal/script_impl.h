#include "tessera/script.h"
#include "tessera_impl.h"
#include "expression.h"
#include "expr_value.h"
#include <vector>
#include <unordered_map>

namespace tess {

    class script::impl_type : public tessera_impl {
        private:
            std::unordered_map<std::string, expr_value> globals_;
            std::vector<std::string> parameters_;
            expr_ptr tableau_;
        public:
            impl_type(const std::vector<std::tuple<std::string, expr_ptr>>& globals, std::vector<std::string>& params, expr_ptr tableau);
    };

}