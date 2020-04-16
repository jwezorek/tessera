#include <memory>
#include <vector>
#include "execution_ctxt.h"
#include "expression.h"
#include "expr_value.h"

namespace tess {

    class var_expr : public expression
    {
    private:
        std::string var_;
    public:
        var_expr(const std::string& var);
        expr_value eval(execution_ctxt& ctx) const override;
    };

    class placeholder_expr : public expression
    {
    private:
        int placeholder_;
    public:
        placeholder_expr(int  placeholder);
        expr_value eval(execution_ctxt& ctx) const override;
    };

    class array_item_expr : public expression
    {
    private:
        expr_ptr ary_;
        expr_ptr index_;
    public:
        array_item_expr(expr_ptr ary, expr_ptr index);
        expr_value eval(execution_ctxt& ctx) const override;
    };

    class func_call_expr : public expression
    {
    private:
        expr_ptr func_;
        std::vector<expr_ptr> args_;
    public:
        func_call_expr(expr_ptr func_, const std::vector<expr_ptr>& args);
        expr_value eval(execution_ctxt& ctx) const override;
    };

    class obj_field_expr : public expression
    {
    private:
        expr_ptr obj_;
        std::string field_;
    public:
        obj_field_expr(expr_ptr obj, std::string field);
        expr_value eval(execution_ctxt& ctx) const override;
    };

}