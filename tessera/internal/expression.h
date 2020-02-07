#pragma once

#include <memory>
#include <vector>
#include <tuple>
#include <unordered_map>
#include <string>
#include <variant>

namespace tess {
    struct eval_ctxt {
        std::unordered_map<std::string, double> variables;
    };

    class expression;
    using expr_ptr = std::shared_ptr<expression>;

    class expression
    {
    public:
        virtual double eval(const eval_ctxt&) const = 0;
    };

    class number_expr : public expression
    {
    private:
        double val_;
    public:
        number_expr(double v);
        double eval(const eval_ctxt&) const override;
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
        double eval(const eval_ctxt& ctxt) const override;
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
        double eval(const eval_ctxt& ctxt) const override;
    };

    class function_call_expr : public expression
    {
    private:
        std::string func_;
        std::vector<expr_ptr> args_;
    public:
        function_call_expr(std::tuple<std::string, std::vector<expr_ptr>> params);
        double eval(const eval_ctxt& ctx) const override;
    };

    struct ary_item {
        std::string name;
        expr_ptr index;
    };

    struct place_holder_ary_item {
        int place_holder;
        expr_ptr index;
    };

    using object_ref_item = std::variant<ary_item, place_holder_ary_item, std::string, int>;

    class object_ref_expr : public expression
    {
    private:
        std::vector<object_ref_item> parts_;
    public:
        object_ref_expr(const std::vector<object_ref_item>& parts);
        double eval(const eval_ctxt& ctxt) const override;
    };

    using expression_params = std::tuple<std::shared_ptr<tess::expression>, std::vector<std::tuple<char, std::shared_ptr<tess::expression>>>>;

    class exponent_expr : public expression
    {
    private:
        expr_ptr base_;
        std::vector<expr_ptr> exponents_;
    public:
        exponent_expr(const expression_params& params);
        double eval(const eval_ctxt& ctxt) const override;
    };

    class addition_expr : public expression
    {
    private:
        std::vector<std::tuple<bool, expr_ptr>> terms_;
    public:
        addition_expr(const expression_params& terms);
        double eval(const eval_ctxt& ctx) const override;
    };

    class multiplication_expr : public expression
    {
    private:
        std::vector<std::tuple<bool, expr_ptr>> factors_;
    public:
        multiplication_expr(const expression_params& terms);
        double eval(const eval_ctxt& ctx) const override;
    };

	class and_expr : public expression
	{
	private:
		std::vector<expr_ptr> conjuncts_;
	public:
		and_expr(const std::vector<expr_ptr> conjuncts);
		double eval(const eval_ctxt& ctx) const override;
	};

	class or_expr : public expression
	{
	private:
		std::vector<expr_ptr> disjuncts_;
	public:
		or_expr(const std::vector<expr_ptr> disjuncts);
		double eval(const eval_ctxt& ctx) const override;
	};

	class equality_expr : public expression
	{
	private:
		std::vector<expr_ptr> operands_;
	public:
		equality_expr(const std::vector<expr_ptr> operands);
		double eval(const eval_ctxt& ctx) const override;
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
		double eval(const eval_ctxt& ctx) const override;
	};
}