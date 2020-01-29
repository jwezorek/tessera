#include "exception.h"
#include <sstream>
#include <boost/range/adaptor/reversed.hpp>

tess::parser::exception::exception(const std::string& stack_item, const std::string& what) :
	what_(what)
{
	stack_.push_back(stack_item);
}

tess::parser::exception::exception(const std::string& stack_item, const std::string& what, std::string::const_iterator where) :
	what_(what), where_(where)
{
	if (!stack_item.empty())
		stack_.push_back(stack_item);
}

bool tess::parser::exception::has_where() const
{
	return where_.has_value();
}

void tess::parser::exception::set_where(std::string::const_iterator w)
{
	where_ = w;
}

std::string tess::parser::exception::what() const
{
	return what_;
}

std::string::const_iterator tess::parser::exception::where() const
{
	return where_.value();
}

void tess::parser::exception::push_stack_item(const std::string& item)
{
	stack_.push_back(item);
}

std::string tess::parser::exception::to_string() const
{
	std::stringstream ss;
	for (const auto& item : boost::adaptors::reverse(stack_)) {
		ss << item << " -> ";
	}
	ss << what_;
	return ss.str();
}
