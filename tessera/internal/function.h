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

    class function : public expression, public std::enable_shared_from_this<function> {
        public:
            expr_value eval(execution_ctxt&) const override;
            const std::vector<std::string>& parameters() const;
            expr_value call(std::vector<expr_value> args, execution_ctxt& ctxt) const;

            function(const  std::vector<std::string>& params, const tile_def& tile_definition);
            function(const  std::vector<std::string>& params, const expr_ptr& body);

        private:
            std::vector<std::string> parameters_;
            std::variant<tile_def, expr_ptr> body_;
    };

}