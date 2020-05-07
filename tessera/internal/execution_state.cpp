#include "execution_state.h"
#include "tessera/error.h"
#include "expr_value.h"
#include "allocator.h"
#include <algorithm>
#include <unordered_set>
#include <iostream>

namespace {

    std::string placeholder(int i) {
        return std::string("$") + std::to_string(i);
    }

}

tess::lex_scope::frame::frame(const std::vector<std::string>& params, const std::vector<expr_value>& args)
{
    if (params.size() != args.size())
        throw tess::error("params/args count mismatch.");
    int n = static_cast<int>(args.size());
    for (int i = 0; i < n; ++i)
        definitions_[params[i]] = args[i];
}

tess::lex_scope::frame::frame(const std::vector<std::tuple<std::string, expr_value>>& assignments)
{
    std::transform(assignments.begin(), assignments.end(), std::inserter(definitions_, definitions_.end()),
        [](const auto& assgn)->std::pair<std::string, expr_value> {
            auto [var, val] = assgn;
            return { var,val };
        }
    );
}

tess::lex_scope::frame::frame(const std::vector<expr_value>& args)
{
    int n = static_cast<int>(args.size());
    for (int i = 0; i < n; i++)
        definitions_[placeholder(i+1)] = args[i];
}

std::optional<tess::expr_value> tess::lex_scope::frame::get(int ph) const
{
    auto i = definitions_.find(placeholder(ph));
    if (i != definitions_.end())
        return i->second;
    else
        return std::nullopt;
}

std::optional<tess::expr_value> tess::lex_scope::frame::get(std::string var) const
{
    auto i = definitions_.find(var);
    if (i != definitions_.end())
        return i->second;
    else
        return std::nullopt;
}

void tess::lex_scope::frame::set(const std::string& var, expr_value val)
{
    definitions_[var] = val;
}

void tess::lex_scope::frame::set(int i, expr_value val)
{
    definitions_[placeholder(i)] = val;
}

void tess::lex_scope::frame::set(const::std::vector<std::string>& vars, const::std::vector<expr_value>& vals)
{
    if (vars.size() != vals.size())
        throw tess::error("scope::frame::set: vars/vals count mismatch.");
    int n = static_cast<int>(vars.size());
    for (int i = 0; i < n; ++i)
        definitions_[vars[i]] = vals[i];
}

tess::lex_scope::frame::iterator tess::lex_scope::frame::begin() const
{
    return definitions_.begin();
}

tess::lex_scope::frame::iterator tess::lex_scope::frame::end() const
{
    return definitions_.end();
}

tess::lex_scope::frame::frame(const std::string& var, expr_value val)
{
    definitions_[var] = val;
}

tess::lex_scope::lex_scope(tess::evaluation_context& ctxt, tess::lex_scope::frame&& ls) :
    ctxt_(ctxt)
{
    ctxt_.push_scope(std::move(ls));
}


tess::lex_scope::lex_scope(evaluation_context& ctxt, const lex_scope::frame& ls) :
    ctxt_(ctxt)
{
    ctxt_.push_scope(ls);
}


tess::lex_scope::~lex_scope()
{
    ctxt_.pop_scope();
}

/*------------------------------------------------------------------------------------------------------*/

std::optional<tess::expr_value> tess::evaluation_context::get_maybe(const std::string& var) const
{
    for (auto i = scopes_.rbegin(); i != scopes_.rend(); ++i) {
        auto maybe_value = i->get(var);
        if (maybe_value.has_value())
            return maybe_value.value();
    }
    return std::nullopt;
}

bool tess::evaluation_context::contains(const std::string& var) const
{
    return get_maybe(var).has_value();
}

bool tess::evaluation_context::contains(int i) const
{
    return contains(placeholder(i));
}

tess::expr_value tess::evaluation_context::get(const std::string& var) const
{
    auto maybe_value = get_maybe(var);
    return (maybe_value.has_value()) ?
        expr_value{ maybe_value.value() }:
        expr_value{ error("Unknown variable: " + var) };
}

tess::expr_value tess::evaluation_context::get(int i) const
{
    return get(placeholder(i));
}

tess::lex_scope::frame& tess::evaluation_context::peek()
{
    return scopes_.back();
}

void tess::evaluation_context::push_scope()
{
    scopes_.push_back({});
}

void tess::evaluation_context::push_scope(lex_scope::frame&& scope)
{
    scopes_.push_back(std::move(scope));
}

void tess::evaluation_context::push_scope(const lex_scope::frame& scope)
{
    scopes_.push_back(scope);
}

tess::lex_scope::frame tess::evaluation_context::pop_scope()
{
    auto frame = scopes_.back();
    scopes_.pop_back();
    return frame;
}

tess::allocator& tess::evaluation_context::allocator()
{
    return state_.allocator();
}

tess::execution_state& tess::evaluation_context::execution_state()
{
    return state_;
}

/*------------------------------------------------------------------------------------------------------*/

class tess::execution_state::impl_type {
public:
    tess::allocator allocator_;
};


tess::execution_state::execution_state() :
    impl_(std::make_shared<impl_type>())
{
}

tess::allocator& tess::execution_state::allocator()
{
    return impl_->allocator_;
}

tess::evaluation_context tess::execution_state::create_eval_context()
{
    return evaluation_context(
        *this
    );
}

tess::evaluation_context tess::execution_state::create_eval_context(const lex_scope::frame& frame)
{
    auto ctxt = create_eval_context();
    ctxt.push_scope(frame);
    return ctxt;
}

