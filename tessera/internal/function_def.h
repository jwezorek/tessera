#pragma once

#include "expression.h"
#include "tile_def.h"
#include <string>
#include <vector>
#include <variant>
#include <memory>

namespace tess {

    class expr_value;
    class eval_context;

    class function_def : public expression {
        public:
            expr_value eval(eval_context&) const override;
            const std::vector<std::string>& parameters() const;

            function_def(const std::vector<std::string>& params, const tile_def& tile_definition);
            function_def(const std::vector<std::string>& params, const expr_ptr& body);
            friend class lambda;

        private:
            std::vector<std::string> parameters_;
            std::variant<std::shared_ptr<tile_def>, expr_ptr> body_;
    };

}