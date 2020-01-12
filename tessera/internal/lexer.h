#pragma once

#include <string>
#include <iterator>
#include <memory>

using input_iterator_type = std::string::const_iterator;

enum class token_type {
	nil,
    tile,
    vertex,
    edge,
    open_curly_brace,
    close_curly_brace,
    open_paren,
    close_paren,
    identifier,
    number,
    cos,
    sin,
    plus,
    minus,
    divide,
    multiply,
    exponent,
    if_statement,
    else_statement,
    lay,
    tableau,
    semicolon,
    eof
};

struct token {

private:
    token_type type_;
    input_iterator_type start_iter_;
    input_iterator_type end_iter_;
    std::string lexeme_;

public:
	token();
	token(token_type tt, input_iterator_type b);
    token(token_type tt, input_iterator_type b, input_iterator_type e );
    bool operator==(const token rhs) const;
    bool is_type(token_type t) const;
    token_type type() const;
    input_iterator_type start_iter() const;
    input_iterator_type end_iter() const;
    std::string lexeme() const;
};

std::ostream& operator<<(std::ostream& os, const token& x);

class lexer {
public:
    lexer(const input_iterator_type& start, const input_iterator_type& end);

    class iterator {
    public:
        using value_type = token;
        using reference = value_type&;
        using pointer = value_type*;
        using iterator_category = std::forward_iterator_tag;
        using difference_type = int;

        iterator(input_iterator_type& start, input_iterator_type& end);
        iterator(input_iterator_type& end);
        iterator() = default;

        iterator operator++();
        iterator operator++(int);
        reference operator*();
        pointer operator->();
        bool operator==(const iterator& rhs) const { return curr_tok_ == rhs.curr_tok_; };
        bool operator!=(const iterator& rhs) const { return !(curr_tok_ == rhs.curr_tok_); };

    private:
        input_iterator_type end_;
        token curr_tok_;
    };


    iterator begin() { return iterator(start_, end_); };
    iterator end() { return iterator(end_); };
private:
    input_iterator_type start_;
    input_iterator_type end_;
};