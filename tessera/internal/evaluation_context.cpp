#include "evaluation_context.h"
#include "execution_state.h"
#include "value.h"
#include "tessera/error.h"
#include <string>
#include <vector>

namespace {

    std::string placeholder(int i) {
        return std::string("$") + std::to_string(i);
    }

}

tess::scope_frame::scope_frame(const std::vector<std::string>& params, const std::vector<expr_value>& args)
{
    if (params.size() != args.size())
        throw tess::error("params/args count mismatch.");
    int n = static_cast<int>(args.size());
    for (int i = 0; i < n; ++i)
        definitions_[params[i]] = args[i];
}

tess::scope_frame::scope_frame(const std::vector<std::tuple<std::string, expr_value>>& assignments)
{
    std::transform(assignments.begin(), assignments.end(), std::inserter(definitions_, definitions_.end()),
        [](const auto& assgn)->std::pair<std::string, expr_value> {
            auto [var, val] = assgn;
            return { var,val };
        }
    );
}

tess::scope_frame::scope_frame(const std::vector<expr_value>& args)
{
    int n = static_cast<int>(args.size());
    for (int i = 0; i < n; i++)
        definitions_[placeholder(i + 1)] = args[i];
}

std::optional<tess::expr_value> tess::scope_frame::get(int ph) const
{
    auto i = definitions_.find(placeholder(ph));
    if (i != definitions_.end())
        return i->second;
    else
        return std::nullopt;
}

std::optional<tess::expr_value> tess::scope_frame::get(std::string var) const
{
    auto i = definitions_.find(var);
    if (i != definitions_.end())
        return i->second;
    else
        return std::nullopt;
}

bool tess::scope_frame::has(std::string str) const
{
    return definitions_.find(str) != definitions_.end();
}

std::vector<tess::expr_value> tess::scope_frame::values() const
{
    std::vector<tess::expr_value> values(definitions_.size());
    std::transform(definitions_.begin(), definitions_.end(), values.begin(),
        [](const auto& def)->expr_value {
            return def.second;
        }
    );
    return values;
}

void tess::scope_frame::set(const std::string& var, expr_value val)
{
    definitions_[var] = val;
}

void tess::scope_frame::set(int i, expr_value val)
{
    definitions_[placeholder(i)] = val;
}

void tess::scope_frame::set(const::std::vector<std::string>& vars, const::std::vector<expr_value>& vals)
{
    if (vars.size() != vals.size())
        throw tess::error("scope::frame::set: vars/vals count mismatch.");
    int n = static_cast<int>(vars.size());
    for (int i = 0; i < n; ++i)
        definitions_[vars[i]] = vals[i];
}

tess::scope_frame::iterator tess::scope_frame::begin() const
{
    return definitions_.begin();
}

tess::scope_frame::iterator tess::scope_frame::end() const
{
    return definitions_.end();
}

std::string tess::scope_frame::to_string() const
{
    return "# scope frame #";
}

tess::scope_frame::scope_frame(const std::string& var, expr_value val)
{
    definitions_[var] = val;
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

bool tess::evaluation_context::has(const std::string& var) const
{
    auto maybe_value = get_maybe(var);
    return maybe_value.has_value();
}

tess::expr_value tess::evaluation_context::get(const std::string& var) const
{
    auto maybe_value = get_maybe(var);

    if (!maybe_value.has_value())
        throw tess::error("Unknown variable: " + var);

    return maybe_value.value();
}

tess::expr_value tess::evaluation_context::get(int i) const
{
    return get(placeholder(i));
}

tess::scope_frame& tess::evaluation_context::peek()
{
    return scopes_.back();
}

void tess::evaluation_context::push_scope()
{
    scopes_.push_back({});
}

void tess::evaluation_context::push_scope(scope_frame&& scope)
{
    scopes_.push_back(std::move(scope));
}

void tess::evaluation_context::push_scope(const scope_frame& scope)
{
    scopes_.push_back(scope);
}

tess::scope_frame tess::evaluation_context::pop_scope()
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

bool tess::evaluation_context::empty() const
{
    return scopes_.empty();
}

int tess::evaluation_context::num_frames() const
{
    return static_cast<int>(scopes_.size());
}
