#pragma once

#include "tableau.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace tess
{
    using inp_range = std::tuple<std::string::const_iterator, std::string::const_iterator>;
    using script_component_specifier = std::tuple<std::string, std::string, std::vector<std::string>, inp_range>;
    using tab_spec = std::tuple<std::string, tess::inp_range>;
	using arguments = std::unordered_map<std::string, std::string>;

    class tessera_script
    {
    private:
        class script_impl_;
        std::shared_ptr<script_impl_> impl_;

    public:
        tessera_script();
        tessera_script(std::vector<script_component_specifier> sections, tab_spec tab);
		std::vector<std::string> parameters() const;
		tableau execute(const arguments& args) const;
    };
}