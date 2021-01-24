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

tess::scope_frame::scope_frame(const std::vector<std::string>& params, const std::vector<value_>& args)
{
    if (params.size() != args.size())
        throw tess::error("params/args count mismatch.");
    int n = static_cast<int>(args.size());
    for (int i = 0; i < n; ++i)
        definitions_[params[i]] = args[i];
}

tess::scope_frame::scope_frame(const std::vector<std::tuple<std::string, value_>>& assignments)
{
    std::transform(assignments.begin(), assignments.end(), std::inserter(definitions_, definitions_.end()),
        [](const auto& assgn)->std::pair<std::string, value_> {
            auto [var, val] = assgn;
            return { var,val };
        }
    );
}

tess::scope_frame::scope_frame(const std::vector<value_>& args)
{
    int n = static_cast<int>(args.size());
    for (int i = 0; i < n; i++)
        definitions_[placeholder(i + 1)] = args[i];
}

std::optional<tess::value_> tess::scope_frame::get(int ph) const
{
    auto i = definitions_.find(placeholder(ph));
    if (i != definitions_.end())
        return i->second;
    else
        return std::nullopt;
}

std::optional<tess::value_> tess::scope_frame::get(std::string var) const
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

std::vector<tess::value_> tess::scope_frame::values() const
{
    std::vector<tess::value_> values(definitions_.size());
    std::transform(definitions_.begin(), definitions_.end(), values.begin(),
        [](const auto& def)->value_ {
            return def.second;
        }
    );
    return values;
}

void tess::scope_frame::set(const std::string& var, value_ val)
{
    definitions_[var] = val;
}

void tess::scope_frame::set(int i, value_ val)
{
    definitions_[placeholder(i)] = val;
}

void tess::scope_frame::set(const::std::vector<std::string>& vars, const::std::vector<value_>& vals)
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

tess::scope_frame::scope_frame(const std::string& var, value_ val)
{
    definitions_[var] = val;
}
/*
void tess::scope_frame::get_references(std::unordered_set<tess::obj_id> &objects) const {
    for (const auto& [key, val] : definitions_) {
        tess::get_references(val, objects);
    }
}
*/

/*------------------------------------------------------------------------------------------------------*/

std::optional<tess::value_> tess::evaluation_context::get_maybe(const std::string& var) const
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

tess::value_ tess::evaluation_context::get(const std::string& var) const
{
    auto maybe_value = get_maybe(var);

    if (!maybe_value.has_value())
        throw tess::error("Unknown variable: " + var);

    return maybe_value.value();
}

tess::value_ tess::evaluation_context::get(int i) const
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

tess::gc_heap& tess::evaluation_context::allocator()
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

/*
void tess::evaluation_context::get_references(std::unordered_set<tess::obj_id> &objects) const {
    for (const auto& scope : scopes_) {
        scope.get_references(objects);
    }
}
*/

const tess::evaluation_context& tess::context_stack::top() const {
    return impl_.back();
}

bool tess::context_stack::empty() const {
    return impl_.empty();
}

tess::evaluation_context& tess::context_stack::top() {
    auto const_this = const_cast<const tess::context_stack*>(this);
    return *( const_cast<tess::evaluation_context*>( &(const_this->top()) ));
}

void tess::context_stack::pop() {
    impl_.pop_back();
}

void tess::context_stack::push(tess::evaluation_context &&ctxt) {
    impl_.push_back( std::move(ctxt) );
}

tess::memoization_tbl& tess::context_stack::memos()
{
    return memos_;
}

void tess::memoization_tbl::insert(const std::string& key, tess::value_ v)
{
}

bool tess::memoization_tbl::contains(const std::string& key) const
{
    return false;
}

tess::value_ tess::memoization_tbl::get(const std::string& key) const
{
    return tess::value_();
}
