#include "tile_impl.h"
#include "tile_patch_impl.h"
#include "allocator.h"
#include "variant_util.h"
#include "stack_machine.h"
#include "ops.h"
#include "geometry.h"
#include <variant>

namespace {

	tess::expr_value on_method(tess::allocator& a, const std::vector<tess::expr_value>& v) {
		auto patch = std::get<tess::tile_patch>(v[0]);
		auto edge = std::get<tess::edge>(v[1]);
		auto maybe_edge = tess::get_impl(patch)->get_edge_on(edge);
		if (maybe_edge.has_value())
			return { maybe_edge.value() };
		else
			return { tess::nil_val() };
	}

	tess::expr_value get_on_lambda(tess::allocator& a) {
		return {
			a.create<tess::lambda>(
				std::vector<std::string>{ "_edge", "_patch" },
				std::vector<tess::stack_machine::item>{ {
						tess::stack_machine::variable("_edge")
					} , {
						std::make_shared<tess::get_var>()
					} , {
						tess::stack_machine::variable("_patch")
					} , {
						std::make_shared<tess::get_var>()
					} , {
						std::make_shared<tess::val_func_op>(
							2,
							on_method,
							"on_method"
						)
					} 
				},
				std::vector<std::string>{}
			)
		};
	}
}

const std::vector<tess::tile>& tess::tile_patch::impl_type::tiles() const
{
    return tiles_;
}

void tess::tile_patch::impl_type::build_edge_table() const
{
	edge_tbl_.clear();
	for (const auto& tile : tiles_) {
		for (const auto& e : tile.edges()) {

			auto key = get_impl(e)->get_edge_location_indices();
			if (edge_tbl_.find(key) == edge_tbl_.end())
				edge_tbl_[key] = e; 
			else
				throw tess::error("invalid tile patch");
		}
	}
}


tess::tile_patch::impl_type::impl_type(obj_id id, std::vector<tess::tile>& tiles) : tessera_impl(id) {
	for ( auto& t : tiles)
		insert_tile(t);
}

void tess::tile_patch::impl_type::insert_tile( tess::tile& t )
{
	auto* tile = get_impl(t);
	tile->set_parent(this);

	for (auto& v : tile->vertices()) {
		auto* vert = get_impl(v);
		int new_vert_index = vert_tbl_.insert(vert->pos());
		vert->set_location( new_vert_index );
	}

	tiles_.push_back(t);
}


void tess::tile_patch::impl_type::insert_field(const std::string& var, const expr_value& val)
{
	fields_[var] = val;
}

tess::expr_value tess::tile_patch::impl_type::get_method(allocator& allocator, const std::string& field) const
{
	if (field != "on")
		return { nil_val() };

	return get_on_lambda(allocator);
}

tess::expr_value tess::tile_patch::impl_type::get_field(allocator& allocator, const std::string& field) const
{
	auto patch_method = get_method(allocator, field);
	if (!patch_method.is_nil())
		return patch_method;

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
	vert_tbl_.apply_transformation(mat);
}

std::string tess::tile_patch::impl_type::debug() const
{
	std::stringstream ss;
	ss << "{\n";
	for (const auto& t : tiles_) {
		ss << "  " << get_impl(t)->debug() << "\n";
	}
	ss << "}\n\n";
	return ss.str();
}

tess::tile_patch tess::tile_patch::impl_type::flip(allocator& a) const {
	tess::tile_patch clone = a.create<tess::tile_patch>();
	std::unordered_map<obj_id, void*> tbl;
	clone_to(a, tbl, get_impl(clone));
	get_impl(clone)->flip();
	return clone;
}


void  tess::tile_patch::impl_type::flip()  {
	apply(flip_matrix());
	for (auto& tile : tiles_)
		for (edge& e : get_impl(tile)->edges())
			get_impl(e)->flip();
	edge_tbl_.clear();
}

std::optional<tess::edge> tess::tile_patch::impl_type::get_edge_on(const edge& e) const
{
	if (edge_tbl_.empty())
		build_edge_table();

	edge_indices u_v = {
		vert_tbl_.get_index(e.u().pos()),
		vert_tbl_.get_index(e.v().pos())
	};

	auto iter = edge_tbl_.find( u_v );
	if (iter != edge_tbl_.end())
		return iter->second;
	else
		return std::nullopt;
}

void tess::tile_patch::impl_type::get_all_referenced_allocations(std::unordered_set<obj_id>& alloc_set) const
{
	auto key = get_id();
	if (alloc_set.find(key) != alloc_set.end())
		return;
	alloc_set.insert(key);

	for (const auto& tile : tiles_)
		expr_value{ tile }.get_all_referenced_allocations(alloc_set);

	for (const auto& [var, val] : fields_)
		val.get_all_referenced_allocations(alloc_set);
}

void tess::tile_patch::impl_type::clone_to(tess::allocator& allocator, std::unordered_map<obj_id, void*>& orginal_to_clone, tile_patch::impl_type* clone) const
{
	for (const auto& t : tiles_) {
		clone->tiles_.push_back(std::get<tile>(expr_value{ t }.clone(allocator, orginal_to_clone)));
	}
	for (const auto& [var, val] : fields_) {
		clone->fields_[var] = val.clone(allocator, orginal_to_clone);
	}
	clone->vert_tbl_ = vert_tbl_;
}

tess::point tess::tile_patch::impl_type::get_vertex_location(int index) const {
	return vert_tbl_.get_location(index);
}

tess::tile tess::tile_patch::impl_type::join(tess::allocator& a) const
{
	auto points = tess::join(this);
	return a.create<tess::tile>(&a, points);
}

/*---------------------------------------------------------------------------------------------*/

tess::cluster::impl_type::impl_type(obj_id id, const std::vector<expr_value>& values) :
	tessera_impl(id),
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

void tess::cluster::impl_type::get_all_referenced_allocations(std::unordered_set<obj_id>& alloc_set) const
{
	auto key = get_id();
	if (alloc_set.find(key) != alloc_set.end())
		return;
	alloc_set.insert(key);

	for (const auto& val : values_)
		val.get_all_referenced_allocations(alloc_set);
}

void tess::cluster::impl_type::clone_to(tess::allocator& allocator, std::unordered_map<obj_id, void*>& orginal_to_clone, cluster::impl_type* clone) const
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
					for (auto t : patch.tiles()) {
						get_impl(t)->set_parent(nullptr);
						tiles.push_back(t);
					}
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
