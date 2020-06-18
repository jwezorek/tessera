#pragma once

#include <vector>
#include <string>
#include <variant>
#include <memory>
#include "tile.h"
#include "error.h"

namespace tess {

    using result = std::variant<std::vector<tile>, error>;

    class script
    {
        friend class tessera_impl;

    public:
        static std::variant<tess::script, tess::error> interpret(const std::string& script);
        const std::vector<std::string>& parameters() const;
        result execute(const std::vector<std::string>& args) const;

        template <typename... T>
        result execute(T&&... a) const {
            return execute({ std::forward<T>(a)... });
        }

        //bool operator!=(const script& scr) { return impl_ != scr.impl_;  }

        class impl_type;
    private:
        std::shared_ptr<impl_type> impl_;
    };


}