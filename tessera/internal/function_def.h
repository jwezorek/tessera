#pragma once

#include "expression.h"
#include "expr_value.h"
#include "execution_ctxt.h"
#include "tile_def.h"
#include <string>
#include <vector>
#include <variant>
#include <memory>

namespace tess {

    class function_def : public expression, public std::enable_shared_from_this<function_def> {
        public:
            expr_value eval(execution_ctxt&) const override;
            const std::vector<std::string>& parameters() const;

            function_def(const  std::vector<std::string>& params, const tile_def& tile_definition);
            function_def(const  std::vector<std::string>& params, const expr_ptr& body);
            friend class lambda;

        private:
            std::vector<std::string> parameters_;
            std::variant<tile_def, expr_ptr> body_;
    };

}