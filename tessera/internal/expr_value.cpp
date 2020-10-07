#include "variant_util.h"
#include "expr_value.h"
#include "tile_impl.h"
#include "tile_patch_impl.h"
#include "lambda_impl.h"
#include "allocator.h"
#include "execution_state.h"
#include "tessera_impl.h"
#include <sstream>

namespace {

	template<typename T>
	T clone_aux(tess::allocator& allocator, std::unordered_map<tess::obj_id, void*>& orginal_to_clone, T original) {
		auto key = original->get_id();
		T clone_impl = nullptr;

		if (orginal_to_clone.find(key) != orginal_to_clone.end()) {
			clone_impl = reinterpret_cast<T>(orginal_to_clone[key]);
		} else {
			clone_impl = allocator.create<T>();
			orginal_to_clone[key] = clone_impl;
			original->clone_to(allocator, orginal_to_clone, clone_impl);
		}

		return clone_impl;
	}

}

class tess::field_ref::impl_type {
public:
	expr_value obj;
	std::string field;
	impl_type(const expr_value& o, std::string f) : obj(o), field(f)
	{}
};

tess::field_ref::field_ref(const expr_value& obj, std::string field) : impl_(std::make_shared< field_ref::impl_type>(obj, field))
{}

void tess::field_ref::set(const expr_value& val)
{
	tess::insert_field(impl_->obj, impl_->field, val);
}

tess::nil_val::nil_val()
{
}

bool tess::operator==(nil_val lhs, nil_val rhs) {
	return true;
}

bool tess::is_simple_value(expr_value v) 
{ 
	return std::holds_alternative<nil_val>(v) ||
		std::holds_alternative<number>(v) ||
		std::holds_alternative<std::string>(v) ||
		std::holds_alternative<bool>(v);
}

bool tess::is_object_like(expr_value v)
{
	// by "object-like" we mean epression values that may have fields.
	return std::holds_alternative<tile_ptr>(v) ||
		std::holds_alternative<patch_ptr>(v) ||
		std::holds_alternative<vertex_ptr>(v) ||
		std::holds_alternative<edge_ptr>(v) ||
		std::holds_alternative<cluster_ptr>(v) ||
		std::holds_alternative<lambda_ptr>(v);
}

bool tess::is_array_like(expr_value v)
{
	// by "array-like" we mean epression values that may be dereferenced via the [] operator.
	return  std::holds_alternative<patch_ptr>(v) ||
		std::holds_alternative<cluster_ptr>(v);
}

bool tess::is_nil(expr_value v)
{
	return std::holds_alternative<tess::nil_val>(v);
}

tess::expr_value tess::clone_value( allocator& allocator, expr_value v) 
{
	if (is_simple_value(v))
		return v;

	if (std::holds_alternative<field_ref>(v))
		throw tess::error("attempted clone a field ref");
	
	std::unordered_map<obj_id, void*> original_to_clone;
	return clone_value(allocator, original_to_clone, v);
}

tess::expr_value tess::clone_value(allocator& allocator, std::unordered_map<obj_id, void*>& original_to_clone, expr_value v) 
{
	if (is_simple_value(v))
		return v;

	std::variant<tile_ptr, patch_ptr, vertex_ptr, edge_ptr, cluster_ptr, lambda_ptr> obj_variant = variant_cast(v);
	return std::visit(
		[&](auto&& obj)->expr_value { return expr_value{ clone_aux(allocator, original_to_clone, obj) }; },
		obj_variant
	);
}

tess::expr_value tess::get_ary_item(expr_value v, int index)
{
	// patches and clusters can be referenced like an array.
	if (!is_array_like(v))
		throw tess::error("attempted reference to a sub-tile of a value that is not a tile patch.");

	std::variant<patch_ptr, cluster_ptr> ary_variant = variant_cast(v);

	return std::visit(
		[&](auto&& obj)->expr_value { return obj->get_ary_item(index); },
		ary_variant
	);
}

int tess::get_ary_count(expr_value v) 
{
	// patches and clusters can be referenced like an array.
	if (!is_array_like(v))
		return -1;

	std::variant<patch_ptr, cluster_ptr> ary_variant = variant_cast(v);

	return std::visit(
		[&](auto&& obj)->int { return obj->get_ary_count(); },
		ary_variant
	);
}

tess::expr_value tess::get_field(expr_value v, allocator& allocator, const std::string& field)
{
	if (!is_object_like(v)) {
		throw tess::error("attempted reference to field of a non-object: " + field);
	}

	std::variant<tile_ptr, patch_ptr, vertex_ptr, edge_ptr, cluster_ptr, lambda_ptr> obj_variant = variant_cast(v);
	
	return std::visit(
		[&](auto&& obj)->expr_value { return obj->get_field(allocator, field); },
		obj_variant
	);
}

void tess::insert_field(expr_value v, const std::string& var, expr_value val)
{
	if (!is_object_like(v))
		return;
	std::variant<tile_ptr, patch_ptr, vertex_ptr, edge_ptr, cluster_ptr, lambda_ptr> obj_variant = variant_cast(v);
	std::visit(
		[&](auto&& obj) { obj->insert_field(var,val); },
		obj_variant
	);
}

std::unordered_set<tess::obj_id> tess::get_all_referenced_allocations(expr_value v)
{
	std::unordered_set<obj_id> references;
	get_all_referenced_allocations(v, references);
	return references;
}

void tess::get_all_referenced_allocations(expr_value v, std::unordered_set<obj_id>& alloc_set)
{
	if (!is_object_like(v))
		return;
	std::variant<tile_ptr, patch_ptr, vertex_ptr, edge_ptr, cluster_ptr, lambda_ptr> obj_variant = variant_cast(v);
	std::visit(
		[&](auto&& obj) { obj->get_all_referenced_allocations(alloc_set); },
		obj_variant
	);
}

std::string tess::to_string(expr_value v)
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

bool tess::operator==(tess::field_ref lhs, tess::field_ref rhs)
{
	return lhs.impl_ == rhs.impl_;
}

bool tess::operator==(const expr_value& lhs, const expr_value& rhs)
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

bool tess::operator!=(const expr_value& lhs, const expr_value& rhs)
{
	return !(lhs == rhs);
}

tess::expr_value::expr_value() : expr_val_var(tess::nil_val())
{
}

tess::expr_value::expr_value(tile_ptr v) : expr_val_var(v)
{
}

tess::expr_value::expr_value(const_tile_ptr v) : expr_val_var(const_cast<tile_ptr>(v))
{
}

tess::expr_value::expr_value(patch_ptr v) : expr_val_var(v)
{
}

tess::expr_value::expr_value(const_patch_ptr v) : expr_val_var(const_cast<patch_ptr>(v))
{
}

tess::expr_value::expr_value(vertex_ptr v) : expr_val_var(v)
{
}

tess::expr_value::expr_value(const_vertex_ptr v) : expr_val_var(const_cast<vertex_ptr>(v))
{
}


tess::expr_value::expr_value(edge_ptr v) : expr_val_var(v)
{
}

tess::expr_value::expr_value(const_edge_ptr v) : expr_val_var(const_cast<edge_ptr>(v))
{
}

tess::expr_value::expr_value(tess::lambda_ptr v) : expr_val_var(v)
{
}

tess::expr_value::expr_value(const_lambda_ptr v) : expr_val_var(const_cast<lambda_ptr>(v))
{
}

tess::expr_value::expr_value(cluster_ptr v) : expr_val_var(v)
{
}

tess::expr_value::expr_value(const_cluster_ptr v) : expr_val_var(const_cast<cluster_ptr>(v))
{
}

tess::expr_value::expr_value(field_ref v) : expr_val_var(v)
{
}

tess::expr_value::expr_value(nil_val v) : expr_val_var(v)
{
}

tess::expr_value::expr_value(number v) : expr_val_var(v)
{
}

tess::expr_value::expr_value(std::string v) : expr_val_var(v)
{
}

tess::expr_value::expr_value(bool v) : expr_val_var(v)
{
}


