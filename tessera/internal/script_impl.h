#include "tessera/script.h"
#include "tessera_impl.h"
#include "assignment_block.h"
#include "tile_def.h"
#include "expression.h"
#include "expr_value.h"
#include "eval_context.h"
#include <vector>
#include <unordered_map>

namespace tess {

    class script::impl_type : public tessera_impl {
        private:
            assignment_block globals_;
            patch_def tableau_;
        public:
            impl_type(const assignment_block& globals, const patch_def& tableau);
            const std::vector<std::string>& parameters() const;
            assignment_block globals() const;
            patch_def tableau() const;
    };

}