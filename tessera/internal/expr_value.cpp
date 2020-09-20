#include "variant_util.h"
#include "expr_value.h"
#include "tile_impl.h"
#include "tile_patch_impl.h"
#include "allocator.h"
#include "execution_state.h"
#include "tessera_impl.h"
#include <sstream>

namespace {

	template<typename T>
	T clone_aux(tess::allocator& allocator, std::unordered_map<tess::obj_id, void*>& orginal_to_clone, T original) {

		typename T::impl_type* original_impl = get_impl(original);
		auto key = original_impl->get_id();
		typename T::impl_type* clone_impl = nullptr;

		if (orginal_to_clone.find(key) != orginal_to_clone.end()) {
			clone_impl = reinterpret_cast<typename T::impl_type*>(orginal_to_clone[key]);
		} else {
			clone_impl = allocator.create_impl<T>();
			orginal_to_clone[key] = clone_impl;
			original_impl->clone_to(allocator, orginal_to_clone, clone_impl);
		}

		return tess::make_tess_obj<T>(clone_impl);
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
	impl_->obj.insert_field(impl_->field, val);
}

tess::nil_val::nil_val()
{
}

bool tess::expr_value::is_simple_value() const
{ 
	return std::holds_alternative<nil_val>(*this) ||
		std::holds_alternative<number>(*this) ||
		std::holds_alternative<std::string>(*this) ||
		std::holds_alternative<bool>(*this) ||
		std::holds_alternative<error>(*this);
}

bool tess::expr_value::is_object_like() const
{
	// by "object-like" we mean epression values that may have fields.
	return std::holds_alternative<tile>(*this) ||
		std::holds_alternative<tile_patch>(*this) ||
		std::holds_alternative<vertex>(*this) ||
		std::holds_alternative<edge>(*this) ||
		std::holds_alternative<cluster>(*this) ||
		std::holds_alternative<lambda>(*this);
}

bool tess::expr_value::is_array_like() const
{
	// by "array-like" we mean epression values that may be dereferenced via the [] operator.
	return  std::holds_alternative<tile_patch>(*this) ||
		std::holds_alternative<cluster>(*this);
}

bool tess::expr_value::is_valid() const
{
    return ! is_error();
}

bool tess::expr_value::is_error() const
{
	return std::holds_alternative<tess::error>(*this);
}

bool tess::expr_value::is_nil() const
{
	return std::holds_alternative<tess::nil_val>(*this);
}

tess::expr_value tess::expr_value::clone( allocator& allocator ) const
{
	if (is_simple_value())
		return *this;

	if (std::holds_alternative<field_ref>(*this))
		return { tess::error("attempted clone a field ref") };
	
	std::unordered_map<obj_id, void*> original_to_clone;
	return clone(allocator, original_to_clone);
	 
}

tess::expr_value tess::expr_value::clone(allocator& allocator, std::unordered_map<obj_id, void*>& original_to_clone) const
{
	if (is_simple_value())
		return *this;

	std::variant<tile, tile_patch, vertex, edge, cluster, lambda> obj_variant = variant_cast(*this);
	return std::visit(
		[&](auto&& obj)->expr_value { return expr_value{ clone_aux(allocator, original_to_clone, obj) }; },
		obj_variant
	);
}

tess::expr_value tess::expr_value::get_ary_item(int index) const
{
	// patches and clusters can be referenced like an array.
	if (!is_array_like())
		return { error("attempted reference to a sub-tile of a value that is not a tile patch.") };
	auto value = static_cast<expr_val_var>(*this);
	std::variant<tile_patch, cluster> ary_variant = variant_cast(value);

	return std::visit(
		[&](auto&& obj)->expr_value { return get_impl(obj)->get_ary_item(index); },
		ary_variant
	);
}

int tess::expr_value::get_ary_count() const
{
	// patches and clusters can be referenced like an array.
	if (!is_array_like())
		return -1;
	auto value = static_cast<expr_val_var>(*this);
	std::variant<tile_patch, cluster> ary_variant = variant_cast(value);

	return std::visit(
		[&](auto&& obj)->int { return get_impl(obj)->get_ary_count(); },
		ary_variant
	);
}

tess::expr_value tess::expr_value::get_field(allocator& allocator, const std::string& field) const
{
	if (!is_object_like()) {
		return { error("attempted reference to field of a non-object.") };
	}
	auto value = static_cast<expr_val_var>(*this);
	std::variant<tile, tile_patch, vertex, edge, cluster, lambda> obj_variant = variant_cast(value);
	
	return std::visit(
		[&](auto&& obj)->expr_value { return get_impl(obj)->get_field(allocator, field); },
		obj_variant
	);
}

void tess::expr_value::insert_field(const std::string& var, expr_value val) const
{
	if (!is_object_like())
		return;
	std::variant<tile, tile_patch, vertex, edge, cluster, lambda> obj_variant = variant_cast(static_cast<expr_val_var>(*this));
	std::visit(
		[&](auto&& obj) { get_impl(obj)->insert_field(var,val); },
		obj_variant
	);
}

std::unordered_set<tess::obj_id> tess::expr_value::get_all_referenced_allocations() const
{
	std::unordered_set<obj_id> references;
	get_all_referenced_allocations(references);
	return references;
}

void tess::expr_value::get_all_referenced_allocations(std::unordered_set<obj_id>& alloc_set) const
{
	if (!is_object_like())
		return;
	std::variant<tile, tile_patch, vertex, edge, cluster, lambda> obj_variant = variant_cast(static_cast<expr_val_var>(*this));
	std::visit(
		[&](auto&& obj) { get_impl(obj)->get_all_referenced_allocations(alloc_set); },
		obj_variant
	);
}

std::string tess::expr_value::to_string() const
{
	if (std::holds_alternative<tess::number>(*this)) {
		std::stringstream ss;
		ss << "#(" << std::get<tess::number>(*this) << ")";
		return ss.str();
	} else if (std::holds_alternative<tess::nil_val>(*this)) {
		return "#(nil)";
	}
	return "#(some expr value)";
}


