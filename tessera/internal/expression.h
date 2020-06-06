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

    class expression : public std::enable_shared_from_this<expression>
    {
    public:
        virtual void compile(stack_machine::stack& stack) const = 0;
        virtual std::string to_string() const;
        virtual expr_value eval(evaluation_context& ) const = 0;
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
        expr_value eval(evaluation_context& ) const override;
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
        expr_value eval(evaluation_context& ctxt ) const override;
        virtual void compile(stack_machine::stack& stack) const override;
        expr_ptr simplify() const override;
        void get_dependencies(std::unordered_set<std::string>& dependencies) const override;
    };

    enum class special_func {
        sqrt,
        sin,
        cos,
        tan,
        arcsin,
        arccos,
        arctan,
        regular_polygon
    };

    class special_function_expr : public expression
    {
    private:
        special_func func_;
        expr_ptr arg_;
    public:
        special_function_expr(std::tuple<std::string, expr_ptr> param);
        special_function_expr(special_func func, expr_ptr arg);
        expr_value eval(evaluation_context& ctxt ) const override;
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
        expr_value eval(evaluation_context& ctxt ) const override;
        virtual void compile(stack_machine::stack& stack) const override;
        void get_dependencies(std::unordered_set<std::string>& dependencies) const override;
        expr_ptr simplify() const override;
    };

    class addition_expr : public expression
    {
    private:
        std::vector<std::tuple<bool, expr_ptr>> terms_;
    public:
        addition_expr(const expression_params& terms);
        addition_expr(const std::vector<std::tuple<bool, expr_ptr>>& terms);
        expr_value eval(evaluation_context& ctx ) const override;
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
        expr_value eval(evaluation_context& ctx ) const override;
        void compile(stack_machine::stack& stack) const override;
        void get_dependencies(std::unordered_set<std::string>& dependencies) const override;
        expr_ptr simplify() const override;
    };

	class and_expr : public expression
	{
	private:
		std::vector<expr_ptr> conjuncts_;
	public:
		and_expr(const std::vector<expr_ptr>& conjuncts);
        expr_value eval(evaluation_context& ctx ) const override;
        void compile(stack_machine::stack& stack) const override;
        void get_dependencies(std::unordered_set<std::string>& dependencies) const override;
        expr_ptr simplify() const override;
	};

	class or_expr : public expression
	{
	private:
		std::vector<expr_ptr> disjuncts_;
	public:
		or_expr(const std::vector<expr_ptr> disjuncts);
        expr_value eval(evaluation_context& ctx ) const override;
        void compile(stack_machine::stack& stack) const override;
        void get_dependencies(std::unordered_set<std::string>& dependencies) const override;
        expr_ptr simplify() const override;
	};

	class equality_expr : public expression
	{
	private:
		std::vector<expr_ptr> operands_;
	public:
		equality_expr(const std::vector<expr_ptr> operands);
        expr_value eval(evaluation_context& ctx ) const override;
        void compile(stack_machine::stack& stack) const override;
        void get_dependencies(std::unordered_set<std::string>& dependencies) const override;
        expr_ptr simplify() const override;
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
        expr_value eval(evaluation_context& ctx ) const override;
        void compile(stack_machine::stack& stack) const override;
        void get_dependencies(std::unordered_set<std::string>& dependencies) const override;
        expr_ptr simplify() const override;
	};

	class nil_expr : public expression
	{
	public:
		nil_expr();
        expr_value eval(evaluation_context& ctx ) const override;
        expr_ptr simplify() const override;
        void compile(stack_machine::stack& stack) const override;
        void get_dependencies(std::unordered_set<std::string>& dependencies) const override;
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
        expr_value eval(evaluation_context& ctx) const override;
        void compile(stack_machine::stack& stack) const override;
        void get_dependencies(std::unordered_set<std::string>& dependencies) const override;
        expr_ptr simplify() const override;
    };
}