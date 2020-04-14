#pragma once

#include "./tessera/error.h"
#include "tile.h"
#include <string>
#include <vector>
#include <variant>
#include <memory>
#include <string>
#include <unordered_map>

namespace tess
{
	class expression;
    class script_impl;

    using inp_range = std::tuple<std::string::const_iterator, std::string::const_iterator>;
    using script_component_specifier = std::tuple<std::string, std::string, std::vector<std::string>, inp_range>;
    using tab_spec = std::tuple<std::string, std::vector<std::string>, tess::inp_range>;
	using global_vars = std::vector<std::tuple<std::string, std::shared_ptr<expression>>>;
    using result = std::variant<std::vector<tile>, error>;

    class tessera_script
    {
		friend class execution_ctxt;

    private:
        std::shared_ptr<script_impl> impl_;

    public:
        tessera_script();
        tessera_script(std::vector<script_component_specifier> sections, tab_spec tab, global_vars globals);
		const std::vector<std::string>& parameters() const;

		result execute(const std::vector<std::string>& args) const;

        template <typename... T>
        result execute(T&&... a) const {
            return execute({ std::forward<T>(a)... });
        }
    };
}