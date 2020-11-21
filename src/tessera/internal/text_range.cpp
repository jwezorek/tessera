#include "text_range.h"

tess::text_range::text_range(const std::string& str) :
	begin_(str.begin()), end_(str.end())
{
}

tess::text_range::text_range(std::string::const_iterator b, std::string::const_iterator e) :
    begin_(b), end_(e)
{
}

std::string::const_iterator tess::text_range::begin() const
{
    return begin_;
}

std::string::const_iterator tess::text_range::end() const
{
    return end_;
}

int tess::text_range::get_line_number(std::string::const_iterator j) const
{
	int line_number = 1;
	for (auto i = begin_; i < j; ++i)
		if (*i == '\n')
			++line_number;
	return line_number;
}

int tess::text_range::get_line_count() const
{
	return get_line_number(end_);
}


