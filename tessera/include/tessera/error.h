#pragma once
#include <string>
#include <iostream>

namespace tess
{
	class error
	{
	private:
		int line_;
		std::string msg_;
	public:
		error(const std::string& msg, int line);
		const std::string& msg() const;
		int line() const;
	};

	std::ostream& operator<<(std::ostream& os, const  error& e);
}