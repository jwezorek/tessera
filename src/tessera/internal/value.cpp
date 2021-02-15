#include "variant_util.h"
#include "value.h"
#include "tile_impl.h"
#include "tile_patch_impl.h"
#include "lambda_impl.h"
#include "gc_heap.h"
#include "execution_state.h"
#include "tessera_impl.h"
#include "field_ref.h"
#include <sstream>

tess::nil_val::nil_val()
{
}

bool tess::operator==(nil_val lhs, nil_val rhs) {
	return true;
}

tess::value_ tess::from_field_value(const field_value& fv)
{
	return std::visit(
		overloaded{
			[](const tile_graph_ptr& t) {
				return tess::value_{ to_const(to_root_ptr(t)) };
			}, 
			[](const patch_graph_ptr& t) {
				return tess::value_{ to_const(to_root_ptr(t)) };
			},
			[](const edge_graph_ptr& t) {
				return tess::value_{ to_const(to_root_ptr(t)) };
			},
			[](const vertex_graph_ptr& t) {
				return tess::value_{ to_const(to_root_ptr(t)) };
			},
			[](const lambda_graph_ptr& t) {
				return tess::value_{ to_const(to_root_ptr(t)) };
			},
			[](const cluster_graph_ptr& t) {
				return tess::value_{ to_const(to_root_ptr(t)) };
			}, 
			[](auto v) {
				return tess::value_{v };
			}
		},
		fv
	);
}

tess::value_ tess::clone_value(tess::gc_heap& allocator, std::unordered_map<tess::obj_id, std::any>& original_to_clone, const tess::value_& v) {
	using object_t = tess::value_traits<tess::value_>::obj_variant;

	if (is_simple_value(v))
		return v;

	if (std::holds_alternative<field_ref_ptr>(v))
		throw tess::error("attempted clone a field ref");

	object_t obj_variant = variant_cast(v);
	return std::visit(
		[&](auto&& obj)->tess::value_ { return tess::make_value(tess::detail::clone_aux(allocator, original_to_clone, obj)); },
		obj_variant
	);
};

tess::field_value&& tess::copy_field(const field_value& fv)
{
	return std::visit(
		overloaded{
			[](const tess::nil_val& t)->tess::field_value&& { return std::move(tess::field_value{tess::nil_val()}); },
			[](tess::number t)->tess::field_value&& { return std::move(tess::field_value{t}); },
			[](bool t)->tess::field_value&& { return  std::move(tess::field_value{ t}); },
			[](const auto& obj)->tess::field_value&& {
				throw std::runtime_error("attempted to copy a field with and object value");
			}
		},
		fv
	);
}

tess::value_ tess::clone_value( gc_heap& allocator, const value_& v)
{
	std::unordered_map<obj_id, std::any> original_to_clone;
	return clone_value(allocator, original_to_clone, v);
}


tess::value_ tess::get_ary_item(value_ v, int index)
{
	// patches and clusters can be referenced like an array.
	if (!is_array_like(v))
		throw tess::error("attempted reference to a sub-tile of a value that is not a tile patch.");

	std::variant<const_patch_root_ptr, const_cluster_root_ptr> ary_variant = variant_cast(v);

	return std::visit(
		[&](auto&& obj)->value_ { return obj->get_ary_item(index); },
		ary_variant
	);
}

int tess::get_ary_count(value_ v) 
{
	// patches and clusters can be referenced like an array.
	if (!is_array_like(v))
		return -1;

	std::variant<const_patch_root_ptr, const_cluster_root_ptr> ary_variant = variant_cast(v);

	return std::visit(
		[&](auto&& obj)->int { return obj->get_ary_count(); },
		ary_variant
	);
}

tess::value_ tess::get_field(value_ v, gc_heap& allocator, const std::string& field)
{
	if (!is_object_like(v)) {
		throw tess::error("attempted reference to field of a non-object: " + field);
	}

	std::variant<const_tile_root_ptr, const_patch_root_ptr, const_vertex_root_ptr, const_edge_root_ptr, const_cluster_root_ptr, const_lambda_root_ptr> obj_variant = variant_cast(v);
	
	return std::visit(
		[&](auto&& obj)->value_ { return obj->get_field(allocator, field); },
		obj_variant
	);
}

void tess::insert_field(value_ v, const std::string& var, value_ val)
{
	if (!is_object_like(v))
		return;
	std::variant<const_tile_root_ptr, const_patch_root_ptr, const_vertex_root_ptr, const_edge_root_ptr, const_cluster_root_ptr, const_lambda_root_ptr> obj_variant = variant_cast(v);
	std::visit(
		[&](auto obj) { //TODO:GP
			/*
		    using obj_type = decltype(obj);
			using base_type = std::remove_const_t<typename obj_type::value_type>;
			gcpp::deferred_ptr<base_type>(obj)->insert_field(var,val);
			*/
		},
		obj_variant
	);
}

std::string tess::to_string(value_ v)
{
	if (std::holds_alternative<tess::number>(v)) {
		std::stringstream ss;
		ss << "#(" << std::get<tess::number>(v) << ")";
		return ss.str();
	} else if (std::holds_alternative<tess::nil_val>(v)) {
		return "#(nil)";
	}
	return "#(some expr value)";
}


std::string tess::serialize(tess::serialization_state& state, tess::value_ val)
{
	return std::visit(
		overloaded{
			[&state](const const_lambda_root_ptr& lambda) -> std::string {
				return lambda->serialize(state);
			},
			[](nil_val v) -> std::string {
				return "<nil>";
			},
			[](number n) -> std::string {
					std::stringstream ss;
					ss << n;
					return ss.str();
			},
			[](std::string s) -> std::string {
				return "\"" + s + "\"";
			},
			[](bool v) -> std::string {
				return v ? "<true>" : "<false>";
			},
			[](const auto& v) {
				return std::string();
			}
		},
		val
	);
}

std::string tess::serialize(value_ val)
{
	serialization_state state;
	return serialize(state, val);
}

bool tess::operator==(const value_& lhs, const value_& rhs)
{
	return std::visit(
		[&]( auto left_val) -> bool {
			using left_type_t = decltype(left_val);
			if (!std::holds_alternative<left_type_t>(rhs))
				return false;
			return left_val == std::get<left_type_t>(rhs);
		},
		lhs
	);

}

bool tess::operator!=(const value_& lhs, const value_& rhs)
{
	return !(lhs == rhs);
}

tess::serialization_state::serialization_state() : current_id_(0)
{
}

std::optional<tess::obj_id> tess::serialization_state::get_obj(tess::obj_id id) const
{
	auto iter = tbl_.find(id);
	if (iter != tbl_.end())
		return iter->second;
	else
		return std::nullopt;
}

tess::obj_id tess::serialization_state::insert(tess::obj_id id)
{
	tbl_[id] = ++current_id_;
	return current_id_;
}
