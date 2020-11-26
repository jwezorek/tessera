#pragma once
#include <string>
#include <iostream>
#include <optional>
#include <exception>

namespace tess
{
class error : public std::exception
	{
	private:
		std::optional<int> line_;
		std::string msg_;

	public:
		error(std::string msg, int line);
		error(std::string msg);
		const std::string& msg() const;
		std::optional<int> line() const;
		std::string to_string() const;
	};

	std::ostream& operator<<(std::ostream& os, const  error& e);
}