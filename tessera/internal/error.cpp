#include "tessera/error.h"

tess::error::error(const std::string& msg, int line) :
	msg_(msg), line_(line)
{
}

tess::error::error(const std::string& msg)
{
	line_ = 0;
	msg_ = msg;
}

const std::string& tess::error::msg() const
{
	return msg_;
}

int tess::error::line() const
{
	return line_;
}

std::string tess::error::to_string() const
{
	return msg();
}

std::ostream& tess::operator<<(std::ostream& os, const  tess::error& e)
{
	os << e.msg() << ", line #" << e.line();
	return os;
}