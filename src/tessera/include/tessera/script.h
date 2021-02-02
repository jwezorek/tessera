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
        script() {}
        static std::variant<tess::script, tess::error> parse(const std::string& script);
        const std::vector<std::string>& parameters() const;
        result execute(const std::vector<std::string>& args) const;

        template <typename... T>
        result execute(T&&... a) const {
            return execute({ std::forward<T>(a)... });
        }
        class impl_type;
    protected:
        
        script(std::shared_ptr<impl_type>  impl);
        std::shared_ptr<impl_type> impl_;
    };


}