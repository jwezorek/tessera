#include "tile_impl.h"
#include "cluster.h"
#include "variant_util.h"
#include "parser/keywords.h"
#include "allocator.h"
#include <algorithm>
#include "ops.h"

namespace {

	tess::cluster::impl_type* edges_as_cluster(tess::allocator& allocator,  const std::vector<tess::edge::impl_type*>& edges) {
		std::vector<tess::expr_value> cluster_contents(edges.size());
		std::transform(edges.begin(), edges.end(), cluster_contents.begin(),
			[](tess::edge::impl_type* e)->tess::expr_value { return tess::expr_value(e); }
		);
		return allocator.create_implementation<tess::cluster::impl_type*>( cluster_contents );
	}

	template<typename T>
	T* perform_clone(tess::allocator& allocator, std::unordered_map<tess::obj_id, void*>& orginal_to_clone, T* impl)
	{
		tess::expr_value wrapper = tess::expr_value(impl);
		auto unwrapped_clone = std::get<T*>(wrapper.clone(allocator, orginal_to_clone));
		return unwrapped_clone;
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
		vertices_[i] = allocator->create_implementation<vertex::impl_type*>(this, i, vertex_locations[i]);
		edges_[i] = allocator->create_implementation<edge::impl_type*>(this, i, i, (i + 1) % n);
	}
}

const std::vector<tess::vertex::impl_type*>& tess::tile::impl_type::vertices() const
{
    return vertices_;
}

std::vector<tess::vertex::vertex::impl_type*>& tess::tile::impl_type::vertices()
{
	return vertices_;
}

const std::vector<tess::edge::impl_type*>& tess::tile::impl_type::edges() const
{
    return edges_;
}

std::vector<tess::edge::impl_type*>& tess::tile::impl_type::edges()
{
	return edges_;
}

void tess::tile::impl_type::set( std::vector<tess::vertex::impl_type*>&& vertices, std::vector<tess::edge::impl_type*>&& edges )
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
	for (const auto* v : vertices_) {
		auto v_clone = std::get<vertex::impl_type*>(expr_value( v ).clone(allocator, orginal_to_clone));
		clone->vertices_.push_back(v_clone);
	}
	for (const auto& e : edges_) {
		auto e_clone = std::get<edge::impl_type*>(expr_value( e ).clone(allocator, orginal_to_clone));
		clone->edges_.push_back(e_clone);
	}

	if (parent_ != nullptr) {
		clone->parent_ = perform_clone<tile_patch::impl_type>(allocator, orginal_to_clone, parent_);
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

tess::tile::impl_type* tess::tile::impl_type::clone_detached(tess::allocator& a) const
{
	if (is_detached())
		return tess::clone(a, this);
	
	// clone this tile such that its parent is only shallow copied.
	auto tile_value = expr_value( this );
	std::unordered_map<obj_id, void*> original_to_clone;
	auto this_patch_key =  parent_->get_id();
	original_to_clone[this_patch_key] = parent_;
	expr_value clone_expr_value = tile_value.clone(a, original_to_clone);

	//now return the clone with the parent detached.
	auto clone_tile = std::get<tess::tile::impl_type*>(clone_expr_value);
	clone_tile->detach();

	return clone_tile;
}

std::string tess::tile::impl_type::debug() const
{
	std::stringstream ss;
	ss << "{ ";
	for (const auto& e : edges_) {
		ss << e->debug() << " ";
	}
	ss << "}";
	return ss.str();
}

const tess::tile::impl_type* tess::tile::impl_type::get_adjacent_tile(int edge_index) const
{
	if (is_detached())
		return nullptr;

	auto* e = edges_[edge_index];
	int u = e->u()->location_index();
	int v = e->v()->location_index();

	auto maybe_adj_edge = parent_->get_edge_on(v, u);
	if (!maybe_adj_edge)
		return nullptr;

	return maybe_adj_edge->parent();
}


const tess::edge::impl_type* tess::tile::impl_type::get_edge_on(tess::allocator& a,  edge::impl_type* edge) const
{
	if (!is_detached()) {
		return std::get<tess::edge::impl_type*>(parent_->get_on(a, edge));
	}

	//TODO: use an rtree here.
	tess::point u = edge->u()->pos();
	tess::point v = edge->v()->pos();

	static auto eps = static_cast<tess::number>(std::numeric_limits<float>::epsilon());

	for (const auto* e : edges()) {
		auto e_u = e->u()->pos();
		auto e_v = e->v()->pos();

		if (tess::distance(e_u, u) <= eps && tess::distance(e_v, v) <= eps)
			return e;
	}

	return nullptr;;
}

tess::expr_value tess::tile::impl_type::get_on(tess::allocator& a,  std::variant<tess::edge::impl_type*, tess::cluster::impl_type*>& var) const
{
	return std::visit(
		overloaded{
			[&]( tess::edge::impl_type* e) -> expr_value {
				auto maybe_edge = get_edge_on(a,e);
				if (maybe_edge) {
					return expr_value( maybe_edge );
				} else {
					return {};
				}
			},
			[&]( tess::cluster::impl_type* c) -> expr_value {
				const auto& items = c->items();
				std::vector<tess::expr_value> on_edges( items.size() );
				std::transform(items.begin(), items.end(), on_edges.begin(),
					[&](const expr_value& v) -> expr_value {
						std::variant<tess::edge::impl_type*, tess::cluster::impl_type*> var = variant_cast(v);
						return this->get_on(a, var);
					}
				);
				return expr_value(a.create_implementation<tess::cluster::impl_type*>(on_edges) );
			}
		},
		var
	);
}

tess::expr_value tess::tile::impl_type::get_field(const std::string& field) const
{
	if (fields_.find(field) != fields_.end())
		return fields_.at(field);

	return expr_value(nil_val());
}

tess::expr_value tess::tile::impl_type::get_field(allocator& allocator, const std::string& field) const
{
	if (field == parser::keyword(parser::kw::edges)) {
		return expr_value(edges_as_cluster(allocator, edges_) );
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
	for (auto* vertex : vertices_) {
		vertex->apply(mat);
	}
}

tess::tile::impl_type* tess::tile::impl_type::flip(allocator& a) const
{
	tess::tile::impl_type* flippee;
	if (is_detached()) {
		flippee = tess::clone(a, this);
	} else {
		flippee = clone_detached(a);
	}
	flippee->flip();
	return flippee;
}

void tess::tile::impl_type::flip()
{
	apply(flip_matrix());
	for (auto& e : edges_) {
		e->flip();
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
	for (auto* v : vertices_)
		v->set_location(v->pos());
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

const tess::vertex::impl_type* tess::edge::impl_type::u() const
{
    return parent_->vertices().at(u_);
}

const tess::vertex::impl_type* tess::edge::impl_type::v() const
{
    return parent_->vertices().at(v_);
}

tess::vertex::impl_type* tess::edge::impl_type::u() 
{
	return parent_->vertices().at(u_);
}

tess::vertex::impl_type* tess::edge::impl_type::v() 
{
	return parent_->vertices().at(v_);
}

tess::edge::impl_type* tess::edge::impl_type::next_edge() const
{
	return v()->out_edge();
}

tess::edge::impl_type* tess::edge::impl_type::prev_edge() const
{
	return u()->in_edge();
}

tess::expr_value tess::edge::impl_type::get_field(allocator& allocator, const std::string& field) const
{
	//TODO: do something about the raw strings here
	if (field == "next") {
		return expr_value(next_edge());
	}

	if (field == "prev") {
		return expr_value(prev_edge() );
	}

	if (field == "u") {
		return expr_value( u() );
	}

	if (field == "v") {
		return expr_value( v() );
	}

	return get_field(field);
}

tess::expr_value tess::edge::impl_type::get_field(const std::string& field) const
{
	if (fields_.find(field) != fields_.end())
		return fields_.at(field);

	return expr_value(nil_val());
}

void tess::edge::impl_type::insert_field(const std::string& var, const expr_value& val)
{
	fields_[var] = val;
}

const std::map<std::string, tess::expr_value>& tess::edge::impl_type::fields() const
{
	return fields_;
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
		u()->location_index(),
		v()->location_index()
	);
}


std::string tess::edge::impl_type::debug() const
{
	std::stringstream ss;
	ss << "[ " << u_ << ":" << u()->debug()
		<< " -> " << v_ << ":" << v()->debug() << " ]";
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
	clone->parent_ = perform_clone<tile::impl_type>(allocator, orginal_to_clone, parent_);

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
	clone->parent_ = perform_clone<tile::impl_type>(allocator, orginal_to_clone, parent_);
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

//TODO: maybe make some lazy created table for doing this in sublinear time for tiles more than k sides?
tess::edge::impl_type* tess::vertex::impl_type::in_edge() const
{
	auto& edges = parent_->edges();
	auto iter = std::find_if(edges.begin(), edges.end(),
		[this](const auto* e) {
			return e->v() == this;
		}
	);
	return *iter;
}

tess::edge::impl_type* tess::vertex::impl_type::out_edge() const
{
	auto& edges = parent_->edges();
	auto iter = std::find_if(edges.begin(), edges.end(),
		[this](const auto* e) {
			return e->u() == this;
		}
	);
	return *iter;
}


