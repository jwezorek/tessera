#include "tessera/script.h"
#include "tessera_impl.h"
#include "assignment_block.h"
#include "function_def.h"
#include "expression.h"
#include "expr_value.h"
#include "eval_context.h"
#include <vector>
#include <unordered_map>

namespace tess {

    class script::impl_type : public tessera_impl {
        private:
            assignment_block globals_;
            expr_ptr tableau_;
        public:
            impl_type(const assignment_block& globals, const expr_ptr& tableau);
            const std::vector<std::string>& parameters() const;
            assignment_block globals() const;
            expr_ptr tableau() const;
    };

}