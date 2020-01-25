#pragma once

#include <string>
#include <vector>
#include <memory>

namespace tess
{
    using inp_range = std::tuple<std::string::const_iterator, std::string::const_iterator>;
    using script_component_specifier = std::tuple<std::string, std::string, std::vector<std::string>, inp_range>;
    using tab_spec = std::tuple<std::string, tess::inp_range>;

    class tessera_script
    {
    private:
        class script_impl_;
        std::shared_ptr<script_impl_> impl_;

    public:
        tessera_script();
        tessera_script(const std::string& source_code, std::vector<script_component_specifier> sections, tab_spec tab);
    };
}