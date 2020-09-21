#pragma once
#include <string>
#include <iostream>
#include <optional>

namespace tess
{
	class error
	{
	private:
		std::optional<int> line_;
		std::string msg_;

	public:
		error(const std::string& msg, int line);
		error(const std::string& msg);
		const std::string& msg() const;
		std::optional<int> line() const;
		std::string to_string() const;
	};

	std::ostream& operator<<(std::ostream& os, const  error& e);
	bool operator==(tess::error l, tess::error r);
}