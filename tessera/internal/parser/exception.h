#pragma once

#include <string>
#include <optional>

namespace tess {
	namespace parser {

		class exception {
			std::string what_;
			std::optional<std::string::const_iterator> where_;
		public:
			exception(const std::string& what);
			exception(const std::string& what, std::string::const_iterator where);
			std::string what() const;
			bool has_where() const;
			std::string::const_iterator where() const;
		};
	}
}