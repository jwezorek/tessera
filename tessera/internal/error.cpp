#include "tessera/error.h"

tess::error::error(const std::string& msg, int line) :
	msg_(msg), line_(line)
{
}

const std::string& tess::error::msg() const
{
	return msg_;
}

int tess::error::line() const
{
	return line_;
}

std::ostream& tess::operator<<(std::ostream& os, const  tess::error& e)
{
	os << e.msg() << ", line #" << e.line();
	return os;
}