#include "variant_util.h"
#include "value.h"
#include "tile_impl.h"
#include "tile_patch_impl.h"
#include "lambda_impl.h"
#include "allocator.h"
#include "execution_state.h"
#include "tessera_impl.h"
#include "field_ref.h"
#include <sstream>

namespace {

	template <typename T>
	using base_type = typename std::remove_const<typename std::remove_pointer<T>::type>::type;

	template <typename T>
	using mutable_ptr = typename std::add_pointer<base_type<T>>::type;

	template<typename T>
	T clone_aux(tess::allocator& allocator, std::unordered_map<tess::obj_id, void*>& orginal_to_clone, T original) {
		using ptr = mutable_ptr<T>;
		auto key = original->get_id();
		ptr clone_impl = nullptr;

		if (orginal_to_clone.find(key) != orginal_to_clone.end()) {
			clone_impl = reinterpret_cast<ptr>(orginal_to_clone[key]);
		} else {
			clone_impl = allocator.create<T>();
			orginal_to_clone[key] = to_void_star(clone_impl);
			original->clone_to(allocator, orginal_to_clone, clone_impl);
		}

		return clone_impl;
	}

}

tess::nil_val::nil_val()
{
}

bool tess::operator==(nil_val lhs, nil_val rhs) {
	return true;
}

bool tess::is_simple_value(value_ v) 
{ 
	return std::holds_alternative<nil_val>(v) ||
		std::holds_alternative<number>(v) ||
		std::holds_alternative<std::string>(v) ||
		std::holds_alternative<bool>(v);
}

bool tess::is_object_like(value_ v)
{
	// by "object-like" we mean epression values that may have fields.
	return std::holds_alternative<const_tile_ptr>(v) ||
		std::holds_alternative<const_patch_ptr>(v) ||
		std::holds_alternative<const_vertex_ptr>(v) ||
		std::holds_alternative<const_edge_ptr>(v) ||
		std::holds_alternative<const_cluster_ptr>(v) ||
		std::holds_alternative<const_lambda_ptr>(v);
}

bool tess::is_array_like(value_ v)
{
	// by "array-like" we mean epression values that may be dereferenced via the [] operator.
	return  std::holds_alternative<const_patch_ptr>(v) ||
		std::holds_alternative<const_cluster_ptr>(v);
}

bool tess::is_nil(value_ v)
{
	return std::holds_alternative<tess::nil_val>(v);
}

tess::value_ tess::clone_value( allocator& allocator, value_ v) 
{
	if (is_simple_value(v))
		return v;

	if (std::holds_alternative<field_ref_ptr>(v))
		throw tess::error("attempted clone a field ref");
	
	std::unordered_map<obj_id, void*> original_to_clone;
	return clone_value(allocator, original_to_clone, v);
}

tess::value_ tess::clone_value(allocator& allocator, std::unordered_map<obj_id, void*>& original_to_clone, value_ v) 
{
	if (is_simple_value(v))
		return v;

	std::variant<const_tile_ptr, const_patch_ptr, const_vertex_ptr, const_edge_ptr, const_cluster_ptr, const_lambda_ptr> obj_variant = variant_cast(v);
	return std::visit(
		[&](auto&& obj)->value_ { return value_{ clone_aux(allocator, original_to_clone, obj) }; },
		obj_variant
	);
}

tess::value_ tess::get_ary_item(value_ v, int index)
{
	// patches and clusters can be referenced like an array.
	if (!is_array_like(v))
		throw tess::error("attempted reference to a sub-tile of a value that is not a tile patch.");

	std::variant<const_patch_ptr, const_cluster_ptr> ary_variant = variant_cast(v);

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

	std::variant<const_patch_ptr, const_cluster_ptr> ary_variant = variant_cast(v);

	return std::visit(
		[&](auto&& obj)->int { return obj->get_ary_count(); },
		ary_variant
	);
}

tess::value_ tess::get_field(value_ v, allocator& allocator, const std::string& field)
{
	if (!is_object_like(v)) {
		throw tess::error("attempted reference to field of a non-object: " + field);
	}

	std::variant<const_tile_ptr, const_patch_ptr, const_vertex_ptr, const_edge_ptr, const_cluster_ptr, const_lambda_ptr> obj_variant = variant_cast(v);
	
	return std::visit(
		[&](auto&& obj)->value_ { return obj->get_field(allocator, field); },
		obj_variant
	);
}

void tess::insert_field(value_ v, const std::string& var, value_ val)
{
	if (!is_object_like(v))
		return;
	std::variant<const_tile_ptr, const_patch_ptr, const_vertex_ptr, const_edge_ptr, const_cluster_ptr, const_lambda_ptr> obj_variant = variant_cast(v);
	std::visit(
		[&](const auto* obj) { 
			using T = std::remove_const_t<std::remove_pointer_t<decltype(obj)>>;
			const_cast<T*>(obj)->insert_field(var,val); 
		},
		obj_variant
	);
}

std::unordered_set<tess::obj_id> tess::get_references(value_ v)
{
	std::unordered_set<obj_id> references;
    get_references(v, references);
	return references;
}

void tess::get_references(value_ v, std::unordered_set<obj_id>& alloc_set)
{
	if (!is_object_like(v))
		return;
	std::variant<const_tile_ptr, const_patch_ptr, const_vertex_ptr, const_edge_ptr, const_cluster_ptr, const_lambda_ptr> obj_variant = variant_cast(v);
	std::visit(
		[&](auto&& obj) { obj->get_references(alloc_set); },
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
/*
tess::value_::value_() : value_variant(tess::nil_val())
{
}

tess::value_::value_(const_tile_ptr v) : value_variant(v)
{
}

tess::value_::value_(const_patch_ptr v) : value_variant(v)
{
}

tess::value_::value_(const_vertex_ptr v) : value_variant(v)
{
}


tess::value_::value_(const_edge_ptr v) : value_variant(v)
{
}

tess::value_::value_(tess::const_lambda_ptr v) : value_variant(v)
{
}

tess::value_::value_(const_cluster_ptr v) : value_variant(v)
{
}

tess::value_::value_(field_ref_ptr v) : value_variant(v)
{
}

tess::value_::value_(nil_val v) : value_variant(v)
{
}

tess::value_::value_(number v) : value_variant(v)
{
}

tess::value_::value_(std::string v) : value_variant(v)
{
}

tess::value_::value_(bool v) : value_variant(v)
{
}
*/

