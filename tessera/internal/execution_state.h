#pragma once

#include <memory>
#include <map>
#include <vector>
#include <string>
#include <optional>
#include "expr_value.h"

namespace tess {

    class allocator;
    class evaluation_context;

    class lex_scope {
        public:

            class frame{
                public:
                    frame() {};
                    frame(const std::vector<std::string>& param, const std::vector<expr_value>& arg);
                    frame(const std::vector<std::tuple<std::string , expr_value>>& assignments);
                    frame(const std::string& var, expr_value val);
                    frame(const std::vector<expr_value>& arg);
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

            lex_scope(evaluation_context& ctxt, lex_scope::frame&& ls);
            lex_scope(evaluation_context& ctxt, const lex_scope::frame& ls);
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
        lex_scope::frame& peek();
        void push_scope();
        void push_scope(lex_scope::frame&& scope);
        void push_scope(const lex_scope::frame& scope);
        lex_scope::frame pop_scope();
        allocator& allocator();

    private:
        class impl_type;
        std::shared_ptr<impl_type> impl_;

        evaluation_context(std::shared_ptr<impl_type> impl) : 
            impl_(impl)
        {}
    };

    class execution_state
    {
        friend class evaluation_context;
    private:
        class impl_type;
        std::shared_ptr<impl_type> impl_;
    public:
        execution_state();
        allocator& allocator() const;
        evaluation_context create_eval_context();
    };

}