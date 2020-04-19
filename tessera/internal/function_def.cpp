#include "function_def.h"
#include "expr_value.h"
#include <sstream>
#include <variant>

/*
#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.
*/
namespace {
    /*
    std::string generate_id() {
        boost::uuids::uuid uuid = boost::uuids::random_generator()();
        std::stringstream ss;
        ss << uuid;
        return ss.str();
    }
    */
}

tess::expr_value tess::function_def::eval(eval_context&) const
{
    return {};
}

const std::vector<std::string>& tess::function_def::parameters() const
{
    return parameters_;
}

const std::variant<std::shared_ptr<tess::tile_def>, tess::expr_ptr>& tess::function_def::body() const
{
    return body_;
}

void tess::function_def::get_dependencies(std::vector<std::string>& dependencies) const
{
    throw std::runtime_error("TODO");
}

tess::function_def::function_def(const std::vector<std::string>& params, const tile_def& tile_definition) :
    parameters_(params),
    body_(std::make_shared<tile_def>(tile_definition))
{
}

tess::function_def::function_def(const std::vector<std::string>& params, const expr_ptr& body) :
    parameters_(params),
    body_(body)
{
}


