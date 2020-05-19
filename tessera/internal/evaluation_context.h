#pragma once

#include <optional>
#include "expr_value.h"

namespace tess {

    class evaluation_context;

    class scope_frame {
    public:
        scope_frame() {};
        scope_frame(const std::vector<std::string>& param, const std::vector<expr_value>& arg);
        scope_frame(const std::vector<std::tuple<std::string, expr_value>>& assignments);
        scope_frame(const std::string& var, expr_value val);
        scope_frame(const std::vector<expr_value>& arg);
        std::optional<expr_value> get(int ph) const;
        std::optional<expr_value> get(std::string str) const;
        void set(const std::string& var, expr_value val);
        void set(int i, expr_value val);
        void set(const::std::vector<std::string>& vars, const::std::vector<expr_value>& vals);
        using iterator = std::map<std::string, expr_value>::const_iterator;
        iterator begin() const;
        iterator end() const;

    private:
        std::map<std::string, expr_value> definitions_;
    };

    class lex_scope {
    public:
        lex_scope(evaluation_context& ctxt, scope_frame&& ls);
        lex_scope(evaluation_context& ctxt, const scope_frame& ls);
        lex_scope(const lex_scope&) = delete;
        lex_scope& operator=(const lex_scope& other) = delete;
        ~lex_scope();

    private:
        evaluation_context& ctxt_;
    };

    class execution_state;

    class evaluation_context
    {
        friend class execution_state;

        std::optional<expr_value> get_maybe(const std::string& var) const;

    public:
        bool contains(const std::string& var) const;
        bool contains(int i) const;
        expr_value get(const std::string& var) const;
        expr_value get(int i) const;
        scope_frame& peek();
        void push_scope();
        void push_scope(scope_frame&& scope);
        void push_scope(const scope_frame& scope);
        scope_frame pop_scope();
        allocator& allocator();
        execution_state& execution_state();
    private:
        std::vector<scope_frame> scopes_;
        tess::execution_state& state_;

        evaluation_context(tess::execution_state& es) :
            state_(es)
        {}
    };

}