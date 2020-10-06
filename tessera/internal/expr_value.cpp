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
		auto key = original->get_id();
		T clone_impl = nullptr;

		if (orginal_to_clone.find(key) != orginal_to_clone.end()) {
			clone_impl = reinterpret_cast<T>(orginal_to_clone[key]);
		} else {
			clone_impl = allocator.create_implementation<T>();
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
	impl_->obj.insert_field(impl_->field, val);
}

tess::nil_val::nil_val()
{
}

bool tess::operator==(nil_val lhs, nil_val rhs) {
	return true;
}


bool tess::expr_value::is_simple_value() const
{ 
	return std::holds_alternative<nil_val>(*this) ||
		std::holds_alternative<number>(*this) ||
		std::holds_alternative<std::string>(*this) ||
		std::holds_alternative<bool>(*this);
}

bool tess::expr_value::is_object_like() const
{
	// by "object-like" we mean epression values that may have fields.
	return std::holds_alternative<tile::impl_type*>(*this) ||
		std::holds_alternative<tile_patch::impl_type*>(*this) ||
		std::holds_alternative<vertex::impl_type*>(*this) ||
		std::holds_alternative<edge::impl_type*>(*this) ||
		std::holds_alternative<cluster::impl_type*>(*this) ||
		std::holds_alternative<lambda::impl_type*>(*this);
}

bool tess::expr_value::is_array_like() const
{
	// by "array-like" we mean epression values that may be dereferenced via the [] operator.
	return  std::holds_alternative<tile_patch::impl_type*>(*this) ||
		std::holds_alternative<cluster::impl_type*>(*this);
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
		throw tess::error("attempted clone a field ref");
	
	std::unordered_map<obj_id, void*> original_to_clone;
	return clone(allocator, original_to_clone);
	 
}

tess::expr_value tess::expr_value::clone(allocator& allocator, std::unordered_map<obj_id, void*>& original_to_clone) const
{
	if (is_simple_value())
		return *this;

	std::variant<tile::impl_type*, tile_patch::impl_type*, vertex::impl_type*, edge::impl_type*, cluster::impl_type*, lambda::impl_type*> obj_variant = variant_cast(*this);
	return std::visit(
		[&](auto&& obj)->expr_value { return expr_value{ clone_aux(allocator, original_to_clone, obj) }; },
		obj_variant
	);
}

tess::expr_value tess::expr_value::get_ary_item(int index) const
{
	// patches and clusters can be referenced like an array.
	if (!is_array_like())
		throw tess::error("attempted reference to a sub-tile of a value that is not a tile patch.");

	auto value = static_cast<expr_val_var>(*this);
	std::variant<tile_patch::impl_type*, cluster::impl_type*> ary_variant = variant_cast(value);

	return std::visit(
		[&](auto&& obj)->expr_value { return obj->get_ary_item(index); },
		ary_variant
	);
}

int tess::expr_value::get_ary_count() const
{
	// patches and clusters can be referenced like an array.
	if (!is_array_like())
		return -1;
	auto value = static_cast<expr_val_var>(*this);
	std::variant<tile_patch::impl_type*, cluster::impl_type*> ary_variant = variant_cast(value);

	return std::visit(
		[&](auto&& obj)->int { return obj->get_ary_count(); },
		ary_variant
	);
}

tess::expr_value tess::expr_value::get_field(allocator& allocator, const std::string& field) const
{
	if (!is_object_like()) {
		throw tess::error("attempted reference to field of a non-object: " + field);
	}

	auto value = static_cast<expr_val_var>(*this);
	std::variant<tile::impl_type*, tile_patch::impl_type*, vertex::impl_type*, edge::impl_type*, cluster::impl_type*, lambda::impl_type*> obj_variant = variant_cast(value);
	
	return std::visit(
		[&](auto&& obj)->expr_value { return obj->get_field(allocator, field); },
		obj_variant
	);
}

void tess::expr_value::insert_field(const std::string& var, expr_value val) const
{
	if (!is_object_like())
		return;
	std::variant<tile::impl_type*, tile_patch::impl_type*, vertex::impl_type*, edge::impl_type*, cluster::impl_type*, lambda::impl_type*> obj_variant = variant_cast(static_cast<expr_val_var>(*this));
	std::visit(
		[&](auto&& obj) { obj->insert_field(var,val); },
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
	std::variant<tile::impl_type*, tile_patch::impl_type*, vertex::impl_type*, edge::impl_type*, cluster::impl_type*, lambda::impl_type*> obj_variant = variant_cast(static_cast<expr_val_var>(*this));
	std::visit(
		[&](auto&& obj) { obj->get_all_referenced_allocations(alloc_set); },
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

tess::expr_value::expr_value(tile::impl_type* v) : expr_val_var(v)
{
}

tess::expr_value::expr_value(const tile::impl_type* v) : expr_val_var(const_cast<tile::impl_type*>(v))
{
}

tess::expr_value::expr_value(tile_patch::impl_type* v) : expr_val_var(v)
{
}

tess::expr_value::expr_value(const tile_patch::impl_type* v) : expr_val_var(const_cast<tile_patch::impl_type*>(v))
{
}

tess::expr_value::expr_value(vertex::impl_type* v) : expr_val_var(v)
{
}

tess::expr_value::expr_value(const vertex::impl_type* v) : expr_val_var(const_cast<vertex::impl_type*>(v))
{
}


tess::expr_value::expr_value(edge::impl_type* v) : expr_val_var(v)
{
}

tess::expr_value::expr_value(const edge::impl_type* v) : expr_val_var(const_cast<edge::impl_type*>(v))
{
}

tess::expr_value::expr_value(tess::lambda::impl_type* v) : expr_val_var(v)
{
}

tess::expr_value::expr_value(const lambda::impl_type* v) : expr_val_var(const_cast<lambda::impl_type*>(v))
{
}

tess::expr_value::expr_value(cluster::impl_type* v) : expr_val_var(v)
{
}

tess::expr_value::expr_value(const cluster::impl_type* v) : expr_val_var(const_cast<cluster::impl_type*>(v))
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


