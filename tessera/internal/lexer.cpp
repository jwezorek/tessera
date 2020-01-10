#include "lexer.h"

namespace
{
    bool skip_space(input_iterator_type& iter, input_iterator_type& end) {
        while (iter != end) {
            if (!std::isspace(*iter)) break;
            ++iter;
        }
        return iter != end;
    }

    token get_next_token(input_iterator_type i, input_iterator_type end)
    {
        if (!skip_space(i, end))
            return token(token_type::eof);

    }
}

token::token(token_type tt, input_iterator_type b, input_iterator_type e) :
    type_(tt), start_iter_(b), end_iter_(e)
{
    if (b != input_iterator_type() && e != input_iterator_type())
        lexeme_ = std::string(b, e);
};

bool token::operator==(const token rhs) const
{
    return type_ == rhs.type_ &&
        start_iter_ == rhs.start_iter_ &&
        end_iter_ == rhs.end_iter_;
}

bool token::is_type(token_type t) const
{
    return type_ == t;
}

token_type token::type() const
{
    return type_;
}

input_iterator_type token::start_iter() const
{
    return start_iter_;
}

input_iterator_type token::end_iter() const
{
    return end_iter_;
}

std::string token::lexeme() const
{
    return lexeme_;
}

lexer::lexer(const input_iterator_type& start, const input_iterator_type& end):
    start_(start), end_(end)
{
}

lexer::iterator::iterator(input_iterator_type& start, input_iterator_type& end):
    end_(end)
{
    curr_tok_ = get_next_token(start, end_);
}

lexer::iterator::iterator(input_iterator_type& end)
{
    curr_tok_ = token(token_type::eof, end, end);
}

lexer::iterator lexer::iterator::operator++()
{
    curr_tok_ = get_next_token(curr_tok_.end_iter(), end_);
    return *this;
}

lexer::iterator lexer::iterator::operator++(int)
{
    iterator t = *this; 
    ++(*this);
    return t;
}

lexer::iterator::reference lexer::iterator::operator*()
{
    return curr_tok_;
}

lexer::iterator::pointer lexer::iterator::operator->()
{
    return &curr_tok_;
}
