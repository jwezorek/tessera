#pragma once

#include <memory>
#include <vector>
#include <tuple>
#include <unordered_map>
#include <string>
#include <variant>

namespace tess {

    class expression;
    using expr_ptr = std::shared_ptr<expression>;

    class eval_context;
    class expr_value;

    class expression
    {
    public:
        virtual expr_value eval( eval_context& ) const = 0;
    };

    class number_expr : public expression
    {
    private:
        int val_;
    public:
        number_expr(int v);
        number_expr(double v); 
        expr_value eval( eval_context& ) const override;
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
        expr_value eval( eval_context& ctxt ) const override;
    };

    enum class special_func {
        sqrt,
        sin,
        cos,
        tan,
        arcsin,
        arccos,
        arctan
    };

    class special_function_expr : public expression
    {
    private:
        special_func func_;
        expr_ptr arg_;
    public:
        special_function_expr(std::tuple<std::string, expr_ptr> param);
        expr_value eval( eval_context& ctxt ) const override;
    };

    using expression_params = std::tuple<std::shared_ptr<tess::expression>, std::vector<std::tuple<char, std::shared_ptr<tess::expression>>>>;

    class exponent_expr : public expression
    {
    private:
        expr_ptr base_;
        std::vector<expr_ptr> exponents_;
    public:
        exponent_expr(const expression_params& params);
        expr_value eval( eval_context& ctxt ) const override;
    };

    class addition_expr : public expression
    {
    private:
        std::vector<std::tuple<bool, expr_ptr>> terms_;
    public:
        addition_expr(const expression_params& terms);
        expr_value eval( eval_context& ctx ) const override;
    };

    class multiplication_expr : public expression
    {
    private:
        std::vector<std::tuple<bool, expr_ptr>> factors_;
    public:
        multiplication_expr(const expression_params& terms);
        expr_value eval( eval_context& ctx ) const override;
    };

	class and_expr : public expression
	{
	private:
		std::vector<expr_ptr> conjuncts_;
	public:
		and_expr(const std::vector<expr_ptr> conjuncts);
        expr_value eval( eval_context& ctx ) const override;
	};

	class or_expr : public expression
	{
	private:
		std::vector<expr_ptr> disjuncts_;
	public:
		or_expr(const std::vector<expr_ptr> disjuncts);
        expr_value eval( eval_context& ctx ) const override;
	};

	class equality_expr : public expression
	{
	private:
		std::vector<expr_ptr> operands_;
	public:
		equality_expr(const std::vector<expr_ptr> operands);
        expr_value eval( eval_context& ctx ) const override;
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
        expr_value eval( eval_context& ctx ) const override;
	};

	class nil_expr : public expression
	{
	public:
		nil_expr();
        expr_value eval( eval_context& ctx ) const override;
	};

    class if_expr : public expression
    {
    private:
        expr_ptr condition_;
        expr_ptr then_clause_;
        expr_ptr else_clause_;
    public:
        if_expr(std::tuple< expr_ptr, expr_ptr, expr_ptr> exprs);
        expr_value eval(eval_context& ctx) const override;
    };
}