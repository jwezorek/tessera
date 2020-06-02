#pragma once
#include <optional>
#include <memory>
#include <vector>
#include "expression.h"
#include "expr_value.h"

namespace tess {

    std::optional<int> eval_integer_expr(const tess::expr_ptr& expr, tess::evaluation_context& ctxt);

    class var_expr : public expression
    {
    private:
        std::string var_;
    public:
        var_expr(const std::string& var);
        expr_value eval(evaluation_context& ctx) const override;
        void compile(stack_machine::stack& stack) const override;
        std::string to_string() const override;
        void get_dependencies(std::unordered_set<std::string>& dependencies) const override;
        expr_ptr simplify() const override;
    };

    class placeholder_expr : public expression
    {
    private:
        int placeholder_;
    public:
        placeholder_expr(int  placeholder);
        expr_value eval(evaluation_context& ctx) const override;
        void get_dependencies(std::unordered_set<std::string>& dependencies) const override;
        expr_ptr simplify() const override;
    };

    class array_item_expr : public expression
    {
    private:
        expr_ptr ary_;
        expr_ptr index_;
    public:
        array_item_expr(expr_ptr ary, expr_ptr index);
        expr_value eval(evaluation_context& ctx) const override;
        void get_dependencies(std::unordered_set<std::string>& dependencies) const override;
        expr_ptr simplify() const override;
    };

    class func_call_expr : public expression
    {
    private:
        expr_ptr func_;
        std::vector<expr_ptr> args_;
    public:
        func_call_expr(expr_ptr func_, const std::vector<expr_ptr>& args);
        void compile(stack_machine::stack& stack) const override;
        std::string to_string() const override;
        expr_value eval(evaluation_context& ctx) const override;
        void get_dependencies(std::unordered_set<std::string>& dependencies) const override;
        expr_ptr simplify() const override;
    };

    class obj_field_expr : public expression
    {
    private:
        expr_ptr obj_;
        std::string field_;
    public:
        obj_field_expr(expr_ptr obj, std::string field);
        expr_value eval(evaluation_context& ctx) const override;
        std::string to_string() const override;
        void compile(stack_machine::stack& stack) const override;
        void get_dependencies(std::unordered_set<std::string>& dependencies) const override;
        expr_ptr simplify() const override;
    };

}