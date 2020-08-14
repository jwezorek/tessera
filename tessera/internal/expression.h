#pragma once

#include "stack_machine.h"
#include <memory>
#include <vector>
#include <tuple>
#include <unordered_set>
#include <string>
#include <variant>
#include <functional>

namespace tess {

    class expression;
    using expr_ptr = std::shared_ptr<expression>;
    using const_expr_ptr = std::shared_ptr<const expression>;

    class evaluation_context;
    class expr_value;

    using expr_visit_func = std::function<void(const_expr_ptr)>;

    class expression 
    {
    public:
        virtual void compile(stack_machine::stack& stack) const = 0;
        virtual std::string to_string() const = 0;
        virtual expr_ptr simplify() const = 0;
        virtual void get_dependencies(std::unordered_set<std::string>& dependencies) const = 0;
    };

    class number_expr : public expression
    {
    private:
        int val_;
    public:
        number_expr(int v);
        number_expr(double v); 
        void compile(stack_machine::stack& stack) const override;
        std::string to_string() const override;
        expr_ptr simplify() const override;
        void get_dependencies(std::unordered_set<std::string>& dependencies) const override;
    };

    class string_expr : public expression
    {
    private:
        std::string val_;
    public:
        string_expr(std::string str);
        void compile(stack_machine::stack& stack) const override;
        std::string to_string() const override;
        expr_ptr simplify() const override;
        void get_dependencies(std::unordered_set<std::string>& dependencies) const override;
    };

    enum class special_num {
        pi,
        phi,
        root_2
    };

    class special_number_expr : public expression
    {
    private:
        special_num num_;
    public:
        special_number_expr(const std::string& v);
        special_number_expr(special_num which);
        virtual void compile(stack_machine::stack& stack) const override;
        expr_ptr simplify() const override;
        void get_dependencies(std::unordered_set<std::string>& dependencies) const override;
        std::string to_string() const override { return "<TODO>"; }
    };

    enum class special_func {
        sqrt,
        sin,
        cos,
        tan,
        arcsin,
        arccos,
        arctan,
        regular_polygon,
        polygon,
        isosceles_triangle,
        flip,
        join
    };

    class special_function_expr : public expression
    {
    private:
        special_func func_;
        std::vector<expr_ptr> args_;
    public:
        special_function_expr(std::tuple<std::string, expr_ptr> param);
        special_function_expr(special_func func, expr_ptr arg);
        special_function_expr(special_func func, const std::vector<expr_ptr>& args);
        void compile(stack_machine::stack& stack) const override;
        void get_dependencies(std::unordered_set<std::string>& dependencies) const override;
        expr_ptr simplify() const override;
        std::string to_string() const override;
    };

    using expression_params = std::tuple<std::shared_ptr<tess::expression>, std::vector<std::tuple<char, std::shared_ptr<tess::expression>>>>;

    class exponent_expr : public expression
    {
    private:
        expr_ptr base_;
        std::vector<expr_ptr> exponents_;
    public:
        exponent_expr(const expression_params& params);
        exponent_expr(expr_ptr base, const std::vector<expr_ptr>& exponents);
        virtual void compile(stack_machine::stack& stack) const override;
        void get_dependencies(std::unordered_set<std::string>& dependencies) const override;
        expr_ptr simplify() const override;
        std::string to_string() const override { return "<TODO>"; }
    };

    class addition_expr : public expression
    {
    private:
        std::vector<std::tuple<bool, expr_ptr>> terms_;
    public:
        addition_expr(const expression_params& terms);
        addition_expr(const std::vector<std::tuple<bool, expr_ptr>>& terms);
        void compile(stack_machine::stack& stack) const override;
        std::string to_string() const override;
        void get_dependencies(std::unordered_set<std::string>& dependencies) const override;
        expr_ptr simplify() const override;
    };

    class multiplication_expr : public expression
    {
    private:
        std::vector<std::tuple<bool, expr_ptr>> factors_;
    public:
        multiplication_expr(const expression_params& factors);
        multiplication_expr(const std::vector<std::tuple<bool, expr_ptr>>& factors);
        void compile(stack_machine::stack& stack) const override;
        void get_dependencies(std::unordered_set<std::string>& dependencies) const override;
        expr_ptr simplify() const override;
        std::string to_string() const override;
    };

    class bool_lit_expr : public expression
    {
    private:
        bool val_;
    public:
        bool_lit_expr(bool val);
        bool_lit_expr(const std::string& keyword);
        void compile(stack_machine::stack& stack) const override;
        void get_dependencies(std::unordered_set<std::string>& dependencies) const override;
        expr_ptr simplify() const override;
        std::string to_string() const override;
    };

	class and_expr : public expression
	{
	private:
		std::vector<expr_ptr> conjuncts_;
	public:
		and_expr(const std::vector<expr_ptr>& conjuncts);
        void compile(stack_machine::stack& stack) const override;
        void get_dependencies(std::unordered_set<std::string>& dependencies) const override;
        expr_ptr simplify() const override;
        std::string to_string() const override { return "<TODO>"; }
	};

	class or_expr : public expression
	{
	private:
		std::vector<expr_ptr> disjuncts_;
	public:
		or_expr(const std::vector<expr_ptr> disjuncts);
        void compile(stack_machine::stack& stack) const override;
        void get_dependencies(std::unordered_set<std::string>& dependencies) const override;
        expr_ptr simplify() const override;
        std::string to_string() const override { return "<TODO>"; }
	};

	class equality_expr : public expression
	{
	private:
		std::vector<expr_ptr> operands_;
	public:
		equality_expr(const std::vector<expr_ptr> operands);
        void compile(stack_machine::stack& stack) const override;
        void get_dependencies(std::unordered_set<std::string>& dependencies) const override;
        expr_ptr simplify() const override;
        std::string to_string() const override { return "<TODO>"; }
	};

	enum class relation_op
	{
		ne,	
		gt,	
		lt,	
		ge,	
		le
	};

	class relation_expr : public expression
	{
	private:
		expr_ptr lhs_;
		relation_op op_;
		expr_ptr rhs_;
	public:
		relation_expr(std::tuple<expr_ptr, std::string, expr_ptr> param);
        relation_expr(expr_ptr lhs, relation_op op, expr_ptr rhs);
        void compile(stack_machine::stack& stack) const override;
        void get_dependencies(std::unordered_set<std::string>& dependencies) const override;
        expr_ptr simplify() const override;
        std::string to_string() const override { return "<TODO>"; }
	};

	class nil_expr : public expression
	{
	public:
		nil_expr();
        expr_ptr simplify() const override;
        void compile(stack_machine::stack& stack) const override;
        void get_dependencies(std::unordered_set<std::string>& dependencies) const override;
        std::string to_string() const override { return "(nil)"; }
	};

    class if_expr : public expression
    {
    private:
        expr_ptr condition_;
        expr_ptr then_clause_;
        expr_ptr else_clause_;
    public:
        if_expr(std::tuple< expr_ptr, expr_ptr, expr_ptr> exprs);
        if_expr(expr_ptr condition, expr_ptr then_clause, expr_ptr else_clause);
        void compile(stack_machine::stack& stack) const override;
        void get_dependencies(std::unordered_set<std::string>& dependencies) const override;
        expr_ptr simplify() const override;
        std::string to_string() const override;
    };

    class on_expr : public expression
    {
    private:
        expr_ptr patch_expr_;
        expr_ptr arg_expr_;
    public:
        on_expr(expr_ptr patch_expr, expr_ptr arg_expr);
        void compile(stack_machine::stack& stack) const override;
        void get_dependencies(std::unordered_set<std::string>& dependencies) const override;
        expr_ptr simplify() const override;
        std::string to_string() const override;
    };
}