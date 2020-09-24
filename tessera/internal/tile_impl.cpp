#include "tile_impl.h"
#include "cluster.h"
#include "variant_util.h"
#include "parser/keywords.h"
#include "allocator.h"
#include <algorithm>
#include "ops.h"

namespace {

	tess::cluster edges_as_cluster(tess::allocator& allocator,  const std::vector<tess::edge>& edges) {
		std::vector<tess::expr_value> cluster_contents(edges.size());
		std::transform(edges.begin(), edges.end(), cluster_contents.begin(),
			[](const tess::edge& e)->tess::expr_value { return { e }; }
		);
		return allocator.create<tess::cluster>( cluster_contents ); 
	}

	template<typename T>
	typename T::impl_type* perform_clone(tess::allocator& allocator, std::unordered_map<tess::obj_id, void*>& orginal_to_clone, typename T::impl_type* impl)
	{
		tess::expr_value wrapper = { tess::make_tess_obj<T>(impl) };
		auto wrapper_clone = std::get<T>(wrapper.clone(allocator, orginal_to_clone));
		return  tess::get_impl(wrapper_clone);
	}
}

std::vector<std::tuple<tess::number, tess::number>> get_regular_poly_vert_loc(int n) {
	std::vector<std::tuple<tess::number, tess::number>> points(n);
	return points;
}

tess::tile::impl_type::impl_type(obj_id id, tess::allocator* allocator, const std::vector<std::tuple<tess::number, tess::number>>& vertex_locations) :
	tessera_impl(id),
	parent_(nullptr),
	index_(-1)
{
	auto n = static_cast<int>(vertex_locations.size());
	vertices_.resize(n);
	edges_.resize(n);

	for (int i = 0; i < n; ++i) {
		vertices_[i] = allocator->create<tess::vertex>(this, i, vertex_locations[i]);
		edges_[i] = allocator->create<tess::edge>(this, i, i, (i + 1) % n);
	}
}

const std::vector<tess::vertex>& tess::tile::impl_type::vertices() const
{
    return vertices_;
}

std::vector<tess::vertex>& tess::tile::impl_type::vertices()
{
	const auto* ct = this;
	return const_cast<std::vector<tess::vertex>&>(ct->vertices());
}

const std::vector<tess::edge>& tess::tile::impl_type::edges() const
{
    return edges_;
}

std::vector<tess::edge>& tess::tile::impl_type::edges()
{
	const auto* ct = this;
	return const_cast<std::vector<tess::edge>&>(ct->edges());
}

void tess::tile::impl_type::set( std::vector<tess::vertex>&& vertices, std::vector<tess::edge>&& edges )
{
	vertices_ = std::move(vertices);
    edges_ = std::move(edges);
}

void tess::tile::impl_type::insert_field(const std::string& var, const expr_value& val)
{
	fields_[var] = val;
}

void tess::tile::impl_type::get_all_referenced_allocations(std::unordered_set<obj_id>& alloc_set) const
{
	auto key = get_id();
	if (alloc_set.find(key) != alloc_set.end())
		return;
	alloc_set.insert(key);

	for (const auto& edge : edges_) 
		 expr_value{edge}.get_all_referenced_allocations(alloc_set);
	for (const auto& vertex : vertices_) 
		expr_value{vertex}.get_all_referenced_allocations(alloc_set);
	for (const auto& [var, val] : fields_)
		val.get_all_referenced_allocations(alloc_set);
}

void tess::tile::impl_type::clone_to(tess::allocator& allocator, std::unordered_map<obj_id, void*>& orginal_to_clone, tile::impl_type* clone) const
{
	for (const auto& v : vertices_) {
		clone->vertices_.push_back(std::get<vertex>(expr_value{ v }.clone(allocator, orginal_to_clone)));
	}
	for (const auto& e : edges_) {
		clone->edges_.push_back(std::get<edge>(expr_value{ e }.clone(allocator, orginal_to_clone)));
	}

	if (parent_ != nullptr) {
		clone->parent_ = perform_clone<tile_patch>(allocator, orginal_to_clone, parent_);
	} else {
		clone->parent_ = nullptr;
	}

	for (const auto& [var, val] : fields_) {
		clone->fields_[var] = val.clone(allocator, orginal_to_clone);
	}
}

bool tess::tile::impl_type::is_detached() const
{
	return parent_ == nullptr;
}

tess::tile tess::tile::impl_type::clone_detached(tess::allocator& a) const
{
	auto this_tile = tess::make_tess_obj<tess::tile>(this);
	if (is_detached())
		return tess::clone(a, this_tile);
	
	// clone this tile such that its parent is only shallow copied.
	auto tile_value = expr_value{ this_tile };
	std::unordered_map<obj_id, void*> original_to_clone;
	auto this_patch_key =  parent_->get_id();
	original_to_clone[this_patch_key] = parent_;
	expr_value clone_expr_value = tile_value.clone(a, original_to_clone);

	//now return the clone with the parent detached.
	auto clone_tile = std::get<tess::tile>(clone_expr_value);
	get_impl(clone_tile)->detach();
	return clone_tile;
}

std::string tess::tile::impl_type::debug() const
{
	std::stringstream ss;
	ss << "{ ";
	for (const auto& e : edges_) {
		ss << get_impl(e)->debug() << " ";
	}
	ss << "}";
	return ss.str();
}

tess::tile::impl_type* tess::tile::impl_type::get_adjacent_tile(int edge_index) const
{
	if (is_detached())
		return nullptr;

	auto* e = get_impl(edges_[edge_index]);
	int u = get_impl(e->u())->location_index();
	int v = get_impl(e->v())->location_index();

	auto maybe_adj_edge = parent_->get_edge_on(v, u);
	if (!maybe_adj_edge.has_value())
		return nullptr;

	auto adj_edge = maybe_adj_edge.value();
	return get_impl(adj_edge)->parent();
}


std::optional<tess::edge> tess::tile::impl_type::get_edge_on(tess::allocator& a, const edge& edge) const
{
	if (!is_detached()) {
		return std::get<tess::edge>(parent_->get_on(a, edge));
	}

	//TODO: use an rtree here.
	tess::point u = get_impl(edge.u())->pos();
	tess::point v = get_impl(edge.v())->pos();

	static auto eps = static_cast<tess::number>(std::numeric_limits<float>::epsilon());

	for (const auto& e : edges()) {
		auto e_u = get_impl(e.u())->pos();
		auto e_v = get_impl(e.v())->pos();

		if (tess::distance(e_u, u) <= eps && tess::distance(e_v, v) <= eps)
			return e;
	}

	return std::nullopt;
}

tess::expr_value tess::tile::impl_type::get_on(tess::allocator& a, const std::variant<tess::edge, tess::cluster>& var) const
{
	return std::visit(
		overloaded{
			[&](const tess::edge& e) -> expr_value {
				auto maybe_edge = get_edge_on(a,e);
				if (maybe_edge.has_value()) {
					return { maybe_edge.value() };
				} else {
				  return {};
				}
			},
			[&](const tess::cluster& c) -> expr_value {
				const auto& items = c.items();
				std::vector<tess::expr_value> on_edges(c.count());
				std::transform(items.begin(), items.end(), on_edges.begin(),
					[&](const expr_value& v) -> expr_value {
						std::variant<tess::edge, tess::cluster> var = variant_cast(v);
						return this->get_on(a, var);
					}
				);
				return { a.create<tess::cluster>(on_edges) };
			}
		},
		var
	);
}

tess::expr_value tess::tile::impl_type::get_field(const std::string& field) const
{
	if (fields_.find(field) != fields_.end())
		return fields_.at(field);

	return { nil_val() };
}

tess::expr_value tess::tile::impl_type::get_field(allocator& allocator, const std::string& field) const
{
	if (field == parser::keyword(parser::kw::edges)) {
		return { edges_as_cluster(allocator, edges_) };
	}

	auto val = get_field(field);
	if (!std::holds_alternative< nil_val>(val))
		return val;
	else
		throw tess::error(std::string("refrenced undefined tile edge, vertex, or field: ") + field );
}

const std::map<std::string, tess::expr_value>& tess::tile::impl_type::fields() const
{
	return fields_;
}

void tess::tile::impl_type::apply(const matrix& mat)
{
	for (auto& vertex : vertices_) {
		get_impl(vertex)->apply(mat);
	}
}

tess::tile tess::tile::impl_type::flip(allocator& a) const
{
	tess::tile flippee;
	if (is_detached()) {
		auto this_tile = tess::make_tess_obj<tess::tile>(this);
		flippee = tess::clone(a, this_tile);
	} else {
		flippee = clone_detached(a);
	}
	get_impl(flippee)->flip();
	return flippee;
}

void tess::tile::impl_type::flip()
{
	apply(flip_matrix());
	for (auto& e : edges_) {
		get_impl(e)->flip();
	}
}

void tess::tile::impl_type::set_parent(tess::tile_patch::impl_type* parent, int index) {

	if (parent == nullptr)
		throw tess::error("invalid tile parent");
	parent_ = parent;
	index_ = index;
}

void tess::tile::impl_type::detach()
{
	for (auto& v : vertices_)
		get_impl(v)->set_location(v.pos());
	parent_ = nullptr;
	index_ = -1;
}

bool tess::tile::impl_type::has_parent() const {
	return parent_ != nullptr;
}

tess::tile_patch::impl_type* tess::tile::impl_type::parent() const {
	return parent_;
}


/*--------------------------------------------------------------------------------*/

tess::edge::impl_type::impl_type( obj_id id, tile::impl_type* parent, int index, int u, int v) :
	tessera_impl(id),
	parent_(parent),
	index_(index),
	u_(u),
	v_(v)
{}

const tess::vertex& tess::edge::impl_type::u() const
{
    return parent_->vertices().at(u_);
}

const tess::vertex& tess::edge::impl_type::v() const
{
    return parent_->vertices().at(v_);
}

tess::expr_value tess::edge::impl_type::get_field(allocator& allocator, const std::string& field) const
{
	return get_field(field);
}

tess::expr_value tess::edge::impl_type::get_field(const std::string& field) const
{
	if (fields_.find(field) != fields_.end())
		return fields_.at(field);

	return { nil_val() };
}

void tess::edge::impl_type::insert_field(const std::string& var, const expr_value& val)
{
	fields_[var] = val;
}

bool tess::edge::impl_type::has_property(const std::string& prop) const
{
	return std::visit(
		overloaded{
			[&](bool b) -> bool {
				return b;
			},
			[&](const auto& v) -> bool {
				return false;
			}
		},
		get_field( prop )
	);
}

tess::tile::impl_type* tess::edge::impl_type::parent() const
{
	return parent_;
}

void  tess::edge::impl_type::flip()
{
	std::swap(u_, v_);
}

tess::edge_indices tess::edge::impl_type::get_edge_location_indices() const {
	return edge_indices(
		get_impl(u())->location_index(),
		get_impl(v())->location_index()
	);
}


std::string tess::edge::impl_type::debug() const
{
	std::stringstream ss;
	ss << "[ " << u_ << ":" << get_impl(u())->debug()
		<< " -> " << v_ << ":" << get_impl(v())->debug() << " ]";
	return ss.str();
}

void tess::edge::impl_type::get_all_referenced_allocations(std::unordered_set<obj_id>& alloc_set) const
{
	auto key = get_id();
	if (alloc_set.find(key) != alloc_set.end())
		return;
	alloc_set.insert(key);

	parent_->get_all_referenced_allocations(alloc_set);
}

void  tess::edge::impl_type::clone_to(tess::allocator& allocator, std::unordered_map<obj_id, void*>& orginal_to_clone, edge::impl_type* clone) const
{
	clone->index_ = index_;
	clone->u_ = u_;
	clone->v_ = v_;
	clone->parent_ = perform_clone<tile>(allocator, orginal_to_clone, parent_);

	for (const auto& [var, val] : fields_) {
		clone->fields_[var] = val.clone(allocator, orginal_to_clone);
	}
}

/*--------------------------------------------------------------------------------*/

tess::vertex::impl_type::impl_type(obj_id id, tile::impl_type* parent, int n, std::tuple<number, number> loc) :
	tessera_impl(id),
	parent_(parent), index_(n), location_(loc)
{
}

std::tuple<double, double> tess::vertex::impl_type::to_floats() const
{
	auto [x, y] = pos();
    return {
        static_cast<double>(x),
		static_cast<double>(y),
    };
}

tess::point tess::vertex::impl_type::pos() const
{
	return std::visit(
		overloaded{
			[&](int vert_index)->point { 
				auto* patch = this->grandparent();
				if (patch == nullptr)
					throw error("corrupt tile patch");
				return patch->get_vertex_location(vert_index);
			},
			[&](point pt)->point {
				return pt;
			},
		},
		location_
	);
}

tess::expr_value tess::vertex::impl_type::get_field(allocator& allocator, const std::string& field) const
{
	return {};
}

void tess::vertex::impl_type::get_all_referenced_allocations(std::unordered_set<obj_id>& alloc_set) const
{
	auto key = get_id();
	if (alloc_set.find(key) != alloc_set.end())
		return;
	alloc_set.insert(key);

	parent_->get_all_referenced_allocations(alloc_set);
}

void tess::vertex::impl_type::clone_to(tess::allocator& allocator, std::unordered_map<obj_id, void*>& orginal_to_clone, vertex::impl_type* clone) const
{
	clone->index_ = index_;
	clone->location_ = location_;
	clone->parent_ = perform_clone<tile>(allocator, orginal_to_clone, parent_);
};

tess::tile_patch::impl_type* tess::vertex::impl_type::grandparent() const
{
	if (!parent_)
		return nullptr;
	return parent_->parent();
}

void tess::vertex::impl_type::set_location(int n)
{
	location_ = n;
}

void tess::vertex::impl_type::set_location(point pt)
{
	location_ = pt;
}

int tess::vertex::impl_type::location_index() const
{
	return std::get<int>(location_);
}

std::string  tess::vertex::impl_type::debug() const
{
	std::stringstream ss;
	auto [x, y] = pos();
	ss << "{" << index_ << " (" << x << " , " << y << " )} ";
	return ss.str();
}



void tess::vertex::impl_type::apply(const tess::matrix& mat) {

	if (std::holds_alternative<point>(location_)) {
		location_ = apply_matrix(mat, std::get<point>(location_));
	} else {
		throw error("vertex::impl_type::apply called on patch vertex");
	}
}

tess::tile::impl_type* tess::vertex::impl_type::parent() const
{
	return parent_;
}

tess::edge::impl_type* tess::vertex::impl_type::in_edge() const
{
	auto& edges = parent_->edges();
	auto iter = std::find_if(edges.begin(), edges.end(),
		[this](const tess::edge& e) {
			return tess::get_impl(e.v()) == this;
		}
	);
	return get_impl(*iter);
}

tess::edge::impl_type* tess::vertex::impl_type::out_edge() const
{
	auto& edges = parent_->edges();
	auto iter = std::find_if(edges.begin(), edges.end(),
		[this](const tess::edge& e) {
			return tess::get_impl(e.u()) == this;
		}
	);
	return get_impl(*iter);
}


