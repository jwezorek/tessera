#include "function_def.h"
#include "value.h"
#include "tile_impl.h"
#include "execution_state.h"
#include "variant_util.h"
#include "lambda_impl.h"
#include <variant>

namespace {

    unsigned int g_id = 0;

    tess::scope_frame to_scope_frame(const std::map<std::string, tess::field_value>& fields) {
        std::vector<std::string> vars;
        std::vector<tess::value_> vals;
        vars.reserve(fields.size());
        vals.reserve(fields.size());
        for (const auto& [var, val] : fields) {
            vars.push_back(var);
            vals.push_back(from_field_value(val));
        }
        return { vars, vals };
    }
}

tess::detail::lambda_impl::lambda_impl( gc_heap& a, const std::vector<std::string>& params, const std::vector<stack_machine::item>& bod, const std::vector<std::string>& deps) :
    parameters_(params), body_(bod), dependencies_(deps), id_(++g_id)
{
}

void tess::detail::lambda_impl::insert_field(const std::string& var, const value_& val)
{
    closure_[var] = to_field_value(val);
}

tess::value_ tess::detail::lambda_impl::get_field(gc_heap& allocator, const std::string& field) const
{
    auto iter = closure_.find(field);
    if (iter != closure_.end())
        return { from_field_value(iter->second) };
    else
        throw tess::error("referenced unknown lambda closure item: " + field);
}

void tess::detail::lambda_impl::set_id(unsigned int id)
{
    id_ = id;
}

tess::scope_frame remove_self(const tess::scope_frame& frame, const  tess::detail::lambda_impl* self) {
    tess::scope_frame output_frame;
    for (const auto& [k, v] : frame) {
        if (std::holds_alternative<tess::const_lambda_root_ptr>(v)) {
            if (std::get<tess::const_lambda_root_ptr>(v).get() == self) {
                output_frame.set(k, std::string("<self>") );
            } else {
                output_frame.set(k, v);
            }
        } else {
            output_frame.set(k, v);
        }
    }
    return output_frame;
}

void tess::detail::lambda_impl::clone_to(tess::gc_heap& allocator, std::unordered_map<obj_id, std::any>& orginal_to_clone, lambda_raw_ptr mutable_clone) const
{
    mutable_clone->set_id(id_);
    mutable_clone->parameters_ = parameters_;
    mutable_clone->dependencies_ = dependencies_;
    mutable_clone->body_ = body_;

    for (const auto& [var, val] : closure_) {
        auto v = tess::clone_value(allocator, orginal_to_clone, val);
        mutable_clone->closure_[var] = std::move(v); //clone fields
    }
}

std::vector<std::string> tess::detail::lambda_impl::unfulfilled_dependencies() const
{
    std::vector<std::string> depends;
    std::copy_if(dependencies_.begin(), dependencies_.end(), std::back_inserter(depends),
        [&](std::string var) {
            return closure_.find(var) == closure_.end();
        }
    );
    return depends;
}

std::string tess::detail::lambda_impl::serialize(tess::serialization_state& state) const {
    auto global_id = get_id();
    auto serialization_id = state.get_obj(global_id);

    std::stringstream ss;
    if (!serialization_id) {
        auto serialization_id = state.insert(global_id);
        std::string serialized_closure = to_scope_frame(closure_).serialize(state);
        if (serialized_closure.empty())
            return {};
        ss << "<" << serialization_id << ":" << id_ << " " << serialized_closure << ">";
    }  else {
        ss << "<" << *serialization_id << ">";
    }

    return ss.str();
}

tess::detail::lambda_impl::const_closure_iter tess::detail::lambda_impl::begin_closure() const
{
    return closure_.begin();
}

tess::detail::lambda_impl::const_closure_iter tess::detail::lambda_impl::end_closure() const
{
    return closure_.end();
}

const std::vector<std::string>& tess::detail::lambda_impl::parameters() const
{
    return parameters_;
}

const std::vector<std::string>& tess::detail::lambda_impl::dependencies() const
{
    return dependencies_;
}

const std::vector<tess::stack_machine::item>& tess::detail::lambda_impl::body() const
{
    return body_;
}


