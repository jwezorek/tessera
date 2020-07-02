#include "stack_machine.h"
#include "tessera/error.h"
#include "variant_util.h"
#include "evaluation_context.h"
#include "execution_state.h"
#include "expression.h"
#include <sstream>

namespace {

    std::string get_indentation(int n) {
        return (n == 0) ? 
            std::string(): 
            std::string(n * 4, ' ');
    }

    std::string replace(std::string src, std::string from, std::string to)
    {
        std::string output;
        output.reserve(src.length());
        size_t start_pos = 0, pos;
        while ((pos = src.find(from, start_pos)) != std::string::npos) {
            output += src.substr(start_pos, pos - start_pos);
            output += to;
            pos += from.length();
            start_pos = pos;
        }
        output += src.substr(start_pos);
        return output;
    }

    std::vector<std::string> extract_unformatted_lines_from_single_line(std::string line) {
        
        auto str = replace(line, "{", "{;");
        str = replace(str, "}", "};");
        std::replace(str.begin(), str.end(), ';', '\n');

        auto ss = std::stringstream(str);
        std::vector<std::string> lines;
        std::string l;
        while (std::getline(ss, l)) {
            lines.push_back(l);
        }
        return lines;
    }

    std::vector<std::string> extract_unformatted_lines(std::string src)
    {
        auto unformatted = std::stringstream(src);
        std::vector<std::string> lines;
        std::string line;
        while (std::getline(unformatted, line)) {
            auto new_lines = extract_unformatted_lines_from_single_line(line);
            lines.insert(lines.end(), new_lines.begin(), new_lines.end());
        }
        return lines;
    }

}

std::optional<tess::error> tess::stack_machine::op_0::execute(tess::stack_machine::stack& main_stack, tess::stack_machine::stack& operand_stack, tess::stack_machine::context_stack& contexts)
{
    if (number_of_args_ > operand_stack.count())
        return tess::error("operand stack underflow.");
    return execute(operand_stack.pop(number_of_args_), contexts);
}

std::optional<tess::error> tess::stack_machine::op_1::execute(tess::stack_machine::stack& main_stack, tess::stack_machine::stack& operand_stack, tess::stack_machine::context_stack& contexts)
{
    if (number_of_args_ > operand_stack.count())
        return tess::error("operand stack underflow.");
    auto op_result = execute(operand_stack.pop(number_of_args_), contexts);
    if (std::holds_alternative<tess::error>(op_result))
        return std::get<tess::error>(op_result);
    main_stack.push(op_result);
    return std::nullopt;
}

std::optional<tess::error> tess::stack_machine::op_multi::execute(tess::stack_machine::stack& main_stack, tess::stack_machine::stack& operand_stack, tess::stack_machine::context_stack& contexts)
{
    if (number_of_args_ > operand_stack.count())
        return tess::error("operand stack underflow.");
    auto op_result = execute(operand_stack.pop(number_of_args_), contexts);
    if (std::holds_alternative<tess::error>(op_result))
        return std::get<tess::error>(op_result);
    main_stack.push(std::get<std::vector<stack_machine::item>>(op_result));
    return std::nullopt;
}


void tess::stack_machine::stack::push(const tess::stack_machine::item& item)
{
    impl_.push_back(item);
}

tess::stack_machine::item tess::stack_machine::stack::pop()
{
    auto val = impl_.back();
    impl_.pop_back();
    return val;
}

void tess::stack_machine::stack::push(const std::vector<item>& items)
{
    push(items.rbegin(), items.rend());
}

void tess::stack_machine::stack::compile_and_push(const std::vector<tess::expr_ptr>& exprs)
{
    for (auto expr_iter = exprs.rbegin(); expr_iter != exprs.rend(); ++expr_iter)
        (*expr_iter)->compile(*this);
}

std::vector<tess::stack_machine::item> tess::stack_machine::stack::pop(int n)
{
    std::vector<tess::stack_machine::item> output(n);
    std::copy(impl_.rbegin(), impl_.rbegin() + n, output.begin());
    impl_.resize(impl_.size() - n);
    return output;
}


bool tess::stack_machine::stack::empty() const
{
    return impl_.empty();
}

int tess::stack_machine::stack::count() const
{
    return static_cast<int>(impl_.size());
}

std::string tess::stack_machine::stack::to_string() const
{
    std::stringstream ss;

    auto stack = impl_;
    std::reverse(stack.begin(), stack.end());

    for (const auto& it : stack) {
        ss << it.to_string() << "\n";
    }

    return ss.str();
}

std::string tess::stack_machine::stack::to_formatted_string() const
{
    auto lines = extract_unformatted_lines(to_string());
    int indent = 0;
    std::stringstream ss;
    for (auto line : lines) {
        indent -= (line.find('}') != std::string::npos) ? 1 : 0;
        ss << get_indentation(indent) << line << "\n";
        indent += (line.find('{') != std::string::npos) ? 1 : 0;
    }
    return ss.str();
}

std::vector<tess::stack_machine::item> tess::stack_machine::stack::pop_all()
{
    return pop(static_cast<int>(impl_.size()));
}

/*------------------------------------------------------------------------------*/

tess::stack_machine::machine::machine()
{
}


tess::expr_value tess::stack_machine::machine::run(execution_state& state)
{
    auto& contexts = state.context_stack();
    auto& main_stack = state.main_stack();
    auto& operands = state.operand_stack();
    expr_value output;

    contexts.push(state.create_eval_context());

    try {

        while (!main_stack.empty()) {
            auto stack_item = main_stack.pop();
            std::visit(
                overloaded{
                    [&](op_ptr op) {
                        auto maybe_error = op->execute(main_stack, operands, contexts);
                        if (maybe_error.has_value())
                            throw maybe_error.value();
                    },
                    [&](auto val) {
                        operands.push(stack_machine::item{ val });
                    }
                },
                stack_item
            );
        }

        output = std::get<expr_value>(operands.pop());

    } catch (tess::error error) {
        return { error };
    } catch (...) {
        return { tess::error("Unknown error.") };
    }
    return output;
}

std::string tess::stack_machine::item::to_string() const
{
    std::stringstream ss;
    std::visit(
        overloaded{
            [&](op_ptr op) {
                ss << op->to_string();
            },
            [&](const auto& val) {
                ss << val.to_string();
            }
        },
        *this
     );
    return ss.str();
}

