#include "tile_impl.h"
#include "tile_patch_impl.h"
#include "allocator.h"
#include "variant_util.h"
#include <variant>

const std::vector<tess::tile>& tess::tile_patch::impl_type::tiles() const
{
    return tiles_;
}


void tess::tile_patch::impl_type::insert_tile( tess::tile& tile ) 
{
	get_impl(tile)->set_parent(this);
	tiles_.push_back(tile);
}


void tess::tile_patch::impl_type::insert_field(const std::string& var, const expr_value& val)
{
	fields_[var] = val;
}

tess::expr_value tess::tile_patch::impl_type::get_field(allocator& allocator, const std::string& field) const
{
	auto iter = fields_.find(field);
	return (iter != fields_.end()) ?
		tess::expr_value{iter->second} :
		tess::expr_value{tess::error(std::string("referenced undefined tile patch field: ") + field)};
}

tess::expr_value tess::tile_patch::impl_type::get_ary_item(int i) const
{
	return { tiles_.at(i) };
}

int tess::tile_patch::impl_type::get_ary_count() const
{
	return static_cast<int>(tiles_.size());
}

void tess::tile_patch::impl_type::apply(const matrix& mat)
{
	for (auto& tile : tiles_)
		get_impl(tile)->apply(mat);
}

void tess::tile_patch::impl_type::flip()
{
	for (auto& tile : tiles_)
		get_impl(tile)->flip();
}

bool tess::tile_patch::impl_type::is_untouched() const 
{
	return get_impl(tiles_.front())->is_untouched();
}

void tess::tile_patch::impl_type::get_all_referenced_allocations(std::unordered_set<void*>& alloc_set) const
{
	auto ptr = to_void_star(this);
	if (alloc_set.find(ptr) != alloc_set.end())
		return;
	alloc_set.insert(ptr);

	for (const auto& tile : tiles_)
		expr_value{ tile }.get_all_referenced_allocations(alloc_set);

	for (const auto& [var, val] : fields_)
		val.get_all_referenced_allocations(alloc_set);
}

void tess::tile_patch::impl_type::clone_to(tess::allocator& allocator, std::unordered_map<void*, void*>& orginal_to_clone, tile_patch::impl_type* clone) const
{
	for (const auto& t : tiles_) {
		clone->tiles_.push_back(std::get<tile>(expr_value{ t }.clone(allocator, orginal_to_clone)));
	}
	for (const auto& [var, val] : fields_) {
		clone->fields_[var] = val.clone(allocator, orginal_to_clone);
	}
}

tess::point tess::tile_patch::impl_type::get_vertex_location(int index) const {
	throw error("TODO: tess::tile_patch::impl_type::get_vertex_location");
}

/*
void tess::tile_patch::impl_type::debug()
{
	for (auto t : tiles_) 
		get_impl(t)->debug();
}
*/
/*---------------------------------------------------------------------------------------------*/

tess::cluster::impl_type::impl_type(const std::vector<expr_value>& values) :
	values_(values)
{}


tess::expr_value tess::cluster::impl_type::get_field(allocator& allocator, const std::string& field) const
{
	return expr_value();
}


void  tess::cluster::impl_type::insert_field(const std::string& var, const expr_value& val)
{
}

tess::expr_value tess::cluster::impl_type::get_ary_item(int i) const
{
	return values_.at(i);
}

void tess::cluster::impl_type::push_value(expr_value val)
{
	values_.push_back(val);
}

int tess::cluster::impl_type::get_ary_count() const
{
	return static_cast<int>(values_.size());
}

const std::vector<tess::expr_value>& tess::cluster::impl_type::values()
{
	return values_;
}

void tess::cluster::impl_type::get_all_referenced_allocations(std::unordered_set<void*>& alloc_set) const
{
	auto ptr = to_void_star(this);
	if (alloc_set.find(ptr) != alloc_set.end())
		return;
	alloc_set.insert(ptr);

	for (const auto& val : values_)
		val.get_all_referenced_allocations(alloc_set);
}

void tess::cluster::impl_type::clone_to(tess::allocator& allocator, std::unordered_map<void*, void*>& orginal_to_clone, cluster::impl_type* clone) const
{
	for (const auto& value : values_) {
		clone->values_.push_back(value.clone(allocator, orginal_to_clone));
	}
}

std::vector<tess::expr_value>::const_iterator tess::cluster::impl_type::begin() const
{
	return values_.cbegin();
}

std::vector<tess::expr_value>::const_iterator tess::cluster::impl_type::end() const
{
	return values_.cend();
}

int count_tiles(const std::vector<tess::expr_value>& tiles_and_patches) {
	int count = 0;
	for (const auto& tile_or_patch : tiles_and_patches)
		std::visit(
			overloaded{
				[&count](const tess::tile&) { ++count; },
				[&count](const tess::tile_patch& patch) { count += patch.count(); },
				[](auto) { throw tess::error("unknown error"); }
			},
			tile_or_patch
		);
	return count;
}

tess::tile_patch tess::flatten(tess::allocator& a, const std::vector<tess::expr_value>& tiles_and_patches) {
	int n = count_tiles(tiles_and_patches);
	std::vector<tess::tile> tiles;
	tiles.reserve(n);
	for (const auto& tile_or_patch : tiles_and_patches) {
		std::visit(
			overloaded{
				[&tiles](const tess::tile& t) { tiles.push_back(t); },
				[&tiles](const tess::tile_patch& patch) {
					std::copy(patch.tiles().begin(), patch.tiles().end(), std::back_inserter(tiles));
				},
				[](auto) { throw tess::error("unknown error"); }
			},
			tile_or_patch
		);
	}
	auto patch_impl = a.create_impl<tess::tile_patch>();
	for (const auto& tile : tiles) {
		auto copy = tess::clone(a, tile);
		patch_impl->insert_tile(copy);
	}
	return tess::make_tess_obj<tess::tile_patch>(patch_impl);
}
