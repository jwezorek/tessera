#pragma once

#include <memory>
#include <vector>
#include <tuple>
#include <unordered_map>
#include <string>
#include <variant>

namespace tess {
    struct exec_ctxt {
    };

    class statement;
    using stmt_ptr = std::shared_ptr<statement>;

    class statement
    {
    public:
        virtual void execute(exec_ctxt&) const = 0;
    };

}