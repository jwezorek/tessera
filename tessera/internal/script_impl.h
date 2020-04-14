#include "tessera_impl.h"
#include "expression.h"
#include <vector>
#include <unordered_map>

namespace tess {

    class script_impl : public tessera_impl {
    private:
        std::unordered_map<std::string, expr_ptr> globals_;
        std::vector<std::string> parameters_;
        expr_ptr tableau_;
    public:
        script_impl(const std::vector<std::tuple<std::string, expr_ptr>>& globals, std::vector<std::string>& params, expr_ptr tableau);
    };

}