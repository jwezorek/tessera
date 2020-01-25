#include "text_range.h"

namespace {

    int get_line_number(const std::string& input, std::string::const_iterator iter) {
        int line_number = 1;
        for (auto i = input.cbegin(); i <= iter; ++i)
            if (*i == '\n')
                ++line_number;
        return line_number;
    }

}

tess::text_range::text_range(const std::string& inp) :
    text_range(inp, inp.begin(), inp.end())
{
}

tess::text_range::text_range(const std::string& inp, std::string::const_iterator b, std::string::const_iterator e) :
    input_(&inp), begin_(b), end_(e)
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

tess::text_range tess::text_range::sub_range(std::string::const_iterator i1, std::string::const_iterator i2) const
{
    return tess::text_range(*input_, i1, i2);
}

tess::text_range tess::text_range::left_range(std::string::const_iterator i) const
{
    return tess::text_range(*input_, begin_, i);
}

tess::error tess::text_range::make_error(const std::string& msg) const
{
    return tess::error(msg, get_line_number(*input_, end_));
}

const std::string& tess::text_range::str() const
{
    return *input_;
}

