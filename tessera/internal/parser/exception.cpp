#include "exception.h"

tess::parser::exception::exception(const std::string& what) : 
	what_(what)
{}

tess::parser::exception::exception(const std::string& what, std::string::const_iterator where) :
	what_(what), where_(where)
{}

bool tess::parser::exception::has_where() const
{
	return where_.has_value();
}

std::string tess::parser::exception::what() const
{
	return what_;
}

std::string::const_iterator tess::parser::exception::where() const
{
	return where_.value();
}
