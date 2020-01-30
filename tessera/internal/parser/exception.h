#pragma once

#include <string>
#include <optional>
#include <vector>

namespace boost {
	namespace spirit {
		namespace x3 {
			template<typename T> struct expectation_failure;
		}
	}
}

namespace tess {
	namespace parser {

		class exception {
			std::vector<std::string> stack_;
			std::string what_;
			std::optional<std::string::const_iterator> where_;
		public:
			exception(const std::string& stack_item, const std::string& what);
			exception(const std::string& stack_item, const std::string& what, std::string::const_iterator where);
			exception(const std::string& stack_item, const boost::spirit::x3::expectation_failure<std::string::const_iterator>& e);
			std::string what() const;
			bool has_where() const;
			void set_where(std::string::const_iterator);
			std::string::const_iterator where() const;
			void push_stack_item(const std::string& item);
			std::string to_string() const;
		};
	}
}