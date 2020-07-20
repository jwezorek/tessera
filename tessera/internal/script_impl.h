#include "tessera/script.h"
#include "tessera_impl.h"
#include "where_expr.h"
#include "function_def.h"
#include "expression.h"
#include "expr_value.h"
#include "execution_state.h"
#include <vector>
#include <unordered_map>

namespace tess {

    class script::impl_type {
        private:
            assignment_block globals_;
            expr_ptr tableau_;
            execution_state state_;
        public:
            impl_type(const assignment_block& globals, const expr_ptr& tableau);
            const std::vector<std::string>& parameters() const;
            const assignment_block& globals() const;
            execution_state& state();
            expr_ptr tableau() const;
    };

}