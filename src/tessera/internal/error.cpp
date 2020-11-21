#include "tessera/error.h"

tess::error::error(std::string msg, int line) :
	msg_(msg), line_(line)
{
}

tess::error::error(std::string msg) : msg_(msg)
{
}

const std::string& tess::error::msg() const
{
	return msg_;
}


std::optional<int> tess::error::line() const
{
	return line_;
}

std::string tess::error::to_string() const
{
	return msg();
}

std::ostream& tess::operator<<(std::ostream& os, const  tess::error& e)
{
	auto maybe_line = e.line();
	os << e.msg();
	if (maybe_line.has_value())
		 os << ", line #" << maybe_line.value();
	return os;
}