#include "execution_state.h"
#include "tessera/error.h"
#include "expr_value.h"
#include "allocator.h"
#include <algorithm>

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

using scope_stack = std::vector<tess::lex_scope::frame>;
using context_set = std::unordered_set<std::shared_ptr<scope_stack>>;

class tess::execution_state::impl_type
{
private:
    tess::allocator allocator_;
    context_set contexts_;
public:
    tess::allocator& allocator() { return allocator_; }
    context_set& contexts() { return contexts_; }
    void remove_ctxt(context_set::iterator i) {
        contexts_.erase(i);
    }
};

/*------------------------------------------------------------------------------------------------------*/

class tess::evaluation_context::impl_type 
{
private:
    execution_state& state_;
    context_set::iterator scopes_;

public:

    impl_type(execution_state& s, context_set::iterator i) :
        state_(s),
        scopes_(i)
    {
    }

    execution_state& state() {
        return state_;
    }

    scope_stack& scopes() {
        return **scopes_;
    }

    impl_type(const impl_type&) = delete;
    impl_type& operator=(const impl_type& other) = delete;

    ~impl_type()
    {
        state_.impl_->remove_ctxt(scopes_);
    }
};

/*------------------------------------------------------------------------------------------------------*/

std::optional<tess::expr_value> tess::evaluation_context::get_maybe(const std::string& var) const
{
    auto& scope_stack = impl_->scopes();
    for (auto i = scope_stack.rbegin(); i != scope_stack.rend(); ++i) {
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
    return impl_->scopes().back();
}

void tess::evaluation_context::push_scope()
{
    impl_->scopes().push_back({});
}

void tess::evaluation_context::push_scope(lex_scope::frame&& scope)
{
    impl_->scopes().push_back(std::move(scope));
}

void tess::evaluation_context::push_scope(const lex_scope::frame& scope)
{
    impl_->scopes().push_back(scope);
}

tess::lex_scope::frame tess::evaluation_context::pop_scope()
{
    auto& scope_stack = impl_->scopes();
    auto frame = scope_stack.back();
    scope_stack.pop_back();
    return frame;
}

tess::allocator& tess::evaluation_context::allocator()
{
    return impl_->state().allocator();
}

/*------------------------------------------------------------------------------------------------------*/

tess::execution_state::execution_state() :
    impl_(std::make_shared<impl_type>())
{
}

tess::allocator& tess::execution_state::allocator() const
{
    return impl_->allocator();
}

tess::evaluation_context tess::execution_state::create_eval_context()
{
    auto [ctxt_impl_iter, dummy] = impl_->contexts().insert(
        std::make_shared<scope_stack>()
    );
    return evaluation_context(
        std::make_shared<evaluation_context::impl_type>(*this, ctxt_impl_iter)
    );
}
