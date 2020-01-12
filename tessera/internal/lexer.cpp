#include "lexer.h"
#include <regex>
#include <optional>
#include <tuple>
#include <unordered_map>

namespace
{

	static std::vector<std::tuple<std::string, token_type>> keywords = {
			{ "tile", token_type::tile},
			{ "vertex", token_type::vertex},
			{ "edge", token_type::edge},
			{ "if", token_type::if_statement},
			{ "else", token_type::else_statement},
			{ "lay", token_type::lay},
			{ "tableau", token_type::tableau},
	};

    bool skip_space(input_iterator_type& iter, input_iterator_type& end) 
	{
        while (iter != end) {
            if (!std::isspace(*iter)) break;
            ++iter;
        }
        return iter != end;
    }

    std::optional<token> lex_keyword(input_iterator_type& iter, input_iterator_type& end) 
	{
        for (auto [str, tok] : keywords) {
            auto keyword_sz = str.size();
            if (end - iter < keyword_sz)
                continue;
			if (std::string(iter, iter + keyword_sz) == str) {
				token t(tok, iter, iter + keyword_sz);
				return t;
			}
        }
        return std::nullopt;
    }

    token get_next_token(input_iterator_type i, input_iterator_type end)
    {
        if (!skip_space(i, end))
            return token(token_type::eof, end);
        auto keyword = lex_keyword(i, end);
        if (keyword.has_value())
            return keyword.value();
        return token();
    }
}

std::ostream& operator<<(std::ostream& os, const token& x)
{
	static std::unordered_map<token_type, std::string> tok_to_str;
	if (tok_to_str.empty())
		for (auto [str, tok] : keywords)
			tok_to_str[tok] = str;
	if (tok_to_str.find(x.type()) != tok_to_str.end()) {
		os << "<" << tok_to_str[x.type()] << ">";
		return os;
	}
	os << "<>";
	return os;
}

token::token() : type_(token_type::nil)
{
}

token::token(token_type tt, input_iterator_type b) : 
	type_(tt), start_iter_(b), end_iter_(b)
{
}

token::token(token_type tt, input_iterator_type b, input_iterator_type e) :
    type_(tt), start_iter_(b), end_iter_(e)
{
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
    curr_tok_ = get_next_token(curr_tok_.end_iter(), end_);
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


