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
            const std::variant<std::shared_ptr<tile_def>, std::shared_ptr<patch_def>>& impl() const;
            void get_dependencies(std::vector<std::string>& dependencies) const override;

            function_def(const tile_def& tile_definition);
            function_def(const patch_def& tile_definition);
            friend class lambda;

        private:
            std::variant<std::shared_ptr<tile_def>, std::shared_ptr<patch_def>> impl_;
    };

}