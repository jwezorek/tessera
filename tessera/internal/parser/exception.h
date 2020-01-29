#pragma once

#include <string>
#include <optional>
#include <vector>

namespace tess {
	namespace parser {

		class exception {
			std::vector<std::string> stack_;
			std::string what_;
			std::optional<std::string::const_iterator> where_;
		public:
			exception(const std::string& stack_item, const std::string& what);
			exception(const std::string& stack_item, const std::string& what, std::string::const_iterator where);
			std::string what() const;
			bool has_where() const;
			void set_where(std::string::const_iterator);
			std::string::const_iterator where() const;
			void push_stack_item(const std::string& item);
			std::string to_string() const;
		};
	}
}