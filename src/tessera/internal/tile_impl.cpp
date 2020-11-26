#include "tile_impl.h"
#include "cluster.h"
#include "variant_util.h"
#include "parser/keywords.h"
#include "allocator.h"
#include <algorithm>
#include "ops.h"

namespace {

	tess::const_cluster_ptr edges_as_cluster(tess::allocator& allocator,  const std::vector<tess::edge_ptr>& edges) {
		std::vector<tess::value_> cluster_contents(edges.size());
		std::transform(edges.begin(), edges.end(), cluster_contents.begin(),
			[](tess::const_edge_ptr e)->tess::value_ { return tess::value_(e); }
		);
		return allocator.create<tess::const_cluster_ptr>( cluster_contents );
	}

	template<typename T>
	T* clone_object(tess::allocator& allocator, std::unordered_map<tess::obj_id, void*>& orginal_to_clone, T* impl)
	{
		tess::value_ wrapper = tess::value_(impl);
		return tess::get_mutable<const T*>(tess::clone_value(allocator, orginal_to_clone, wrapper));
	}
}

tess::detail::tile_impl::tile_impl(obj_id id, tess::allocator* allocator, const std::vector<std::tuple<tess::number, tess::number>>& vertex_locations) :
	tessera_impl(id),
	parent_(nullptr),
	index_(-1)
{
	auto n = static_cast<int>(vertex_locations.size());
	vertices_.resize(n);
	edges_.resize(n);

	for (int i = 0; i < n; ++i) {
		vertices_[i] = allocator->create<const_vertex_ptr>(this, i, vertex_locations[i]);
		edges_[i] = allocator->create<const_edge_ptr>(this, i, i, (i + 1) % n);
	}
}

std::vector<tess::const_vertex_ptr> tess::detail::tile_impl::vertices() const
{
	std::vector<tess::const_vertex_ptr> const_vertices(vertices_.size());
	std::transform(vertices_.begin(), vertices_.end(), const_vertices.begin(), 
		[](auto v) { return v; }
	);
	return const_vertices;
}

std::vector<tess::const_edge_ptr> tess::detail::tile_impl::edges() const
{
	std::vector<tess::const_edge_ptr> const_edges(edges_.size());
	std::transform(edges_.begin(), edges_.end(), const_edges.begin(),
		[](auto v) { return v; }
	);
	return const_edges;
}

const std::vector<tess::vertex_ptr>& tess::detail::tile_impl::vertices() 
{
    return vertices_;
}

const std::vector<tess::edge_ptr>& tess::detail::tile_impl::edges() 
{
    return edges_;
}

tess::const_vertex_ptr tess::detail::tile_impl::vertex(int index) const
{
	return vertices_.at(index);
}

tess::vertex_ptr tess::detail::tile_impl::vertex(int index)
{
	return vertices_.at(index);
}

tess::const_edge_ptr tess::detail::tile_impl::edge(int index) const
{
	return edges_.at(index);
}

tess::edge_ptr tess::detail::tile_impl::edge(int index)
{
	return edges_.at(index);
}

void tess::detail::tile_impl::insert_field(const std::string& var, const value_& val)
{
	fields_[var] = val;
}

void tess::detail::tile_impl::get_all_referenced_allocations(std::unordered_set<obj_id>& alloc_set) const
{
	auto key = get_id();
	if (alloc_set.find(key) != alloc_set.end())
		return;
	alloc_set.insert(key);

	for (const auto& edge : edges_) 
		 tess::get_all_referenced_allocations(value_{ edge }, alloc_set);
	for (const auto& vertex : vertices_) 
		tess::get_all_referenced_allocations(value_{ vertex }, alloc_set);
	for (const auto& [var, val] : fields_)
		tess::get_all_referenced_allocations(val, alloc_set);
}

void tess::detail::tile_impl::clone_to(tess::allocator& allocator, std::unordered_map<obj_id, void*>& orginal_to_clone, tile_ptr mutable_clone) const
{
	for (auto* v : vertices_) {
		mutable_clone->vertices_.push_back( clone_object(allocator, orginal_to_clone, v) );
	}
	for (auto& e : edges_) {
		mutable_clone->edges_.push_back( clone_object(allocator, orginal_to_clone, e) );
	}

	if (parent_ != nullptr) {
		mutable_clone->parent_ = clone_object(allocator, orginal_to_clone, parent_);
	} else {
		mutable_clone->parent_ = nullptr;
	}

	for (const auto& [var, val] : fields_) {
		mutable_clone->fields_[var] = tess::clone_value(allocator, orginal_to_clone, val);
	}
}

bool tess::detail::tile_impl::is_detached() const
{
	return parent_ == nullptr;
}

tess::tile_ptr tess::detail::tile_impl::clone_detached(tess::allocator& a) const
{
	if (is_detached())
		return tess::clone(a, this);
	
	// clone this tile such that its parent is only shallow copied.
	auto tile_value = value_( this );
	std::unordered_map<obj_id, void*> original_to_clone;
	auto this_patch_key =  parent_->get_id();
	original_to_clone[this_patch_key] = to_void_star(parent_);
	value_ clone_value_ = tess::clone_value(a, original_to_clone, tile_value);

	//now return the clone with the parent detached.
	auto clone_tile = get_mutable<tess::const_tile_ptr>(clone_value_);
	clone_tile->detach();

	return clone_tile;
}

std::string tess::detail::tile_impl::debug() const
{
	std::stringstream ss;
	ss << "{ ";
	for (const auto& e : edges_) {
		ss << e->debug() << " ";
	}
	ss << "}";
	return ss.str();
}

tess::const_tile_ptr tess::detail::tile_impl::get_adjacent_tile(int edge_index) const
{
	if (is_detached())
		return nullptr;

	auto* e = edges_[edge_index];
	int u = e->u()->location_index();
	int v = e->v()->location_index();

	auto adj_edge = parent_->get_edge_on(v, u);
	return (adj_edge) ? adj_edge->parent() : nullptr;
}

tess::const_edge_ptr tess::detail::tile_impl::get_edge_on(tess::allocator& a,  const_edge_ptr edge) const
{
	if (!is_detached()) {
		return std::get<tess::const_edge_ptr>(parent_->get_on(a, edge));
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

tess::value_ tess::detail::tile_impl::get_on(tess::allocator& a,  std::variant<tess::const_edge_ptr, tess::const_cluster_ptr>& var) const
{
	return std::visit(
		overloaded{
			[&]( tess::const_edge_ptr e) -> value_ {
				auto maybe_edge = get_edge_on(a,e);
				if (maybe_edge) {
					return value_( maybe_edge );
				} else {
					return {};
				}
			},
			[&]( tess::const_cluster_ptr c) -> value_ {
				const auto& items = c->items();
				std::vector<tess::value_> on_edges( items.size() );
				std::transform(items.begin(), items.end(), on_edges.begin(),
					[&](const value_& v) -> value_ {
						std::variant<tess::const_edge_ptr, tess::const_cluster_ptr> var = variant_cast(v);
						return this->get_on(a, var);
					}
				);
				return value_(a.create<tess::const_cluster_ptr>(on_edges) );
			}
		},
		var
	);
}

tess::value_ tess::detail::tile_impl::get_field(const std::string& field) const
{
	if (fields_.find(field) != fields_.end())
		return fields_.at(field);

	return value_(nil_val());
}

tess::value_ tess::detail::tile_impl::get_field(allocator& allocator, const std::string& field) const
{
	if (field == parser::keyword(parser::kw::edges)) {
		return value_(edges_as_cluster(allocator, edges_) );
	}

	auto val = get_field(field);
	if (!std::holds_alternative< nil_val>(val))
		return val;
	else
		throw tess::error(std::string("refrenced undefined tile edge, vertex, or field: ") + field );
}

const std::map<std::string, tess::value_>& tess::detail::tile_impl::fields() const
{
	return fields_;
}

void tess::detail::tile_impl::apply(const matrix& mat)
{
	for (auto* vertex : vertices_) {
		vertex->apply(mat);
	}
}

tess::const_tile_ptr tess::detail::tile_impl::flip(allocator& a) const
{
	tess::tile_ptr flippee;
	if (is_detached()) {
		flippee = tess::clone(a, this);
	} else {
		flippee = clone_detached(a);
	}
	flippee->flip();
	return flippee;
}

void tess::detail::tile_impl::flip()
{
	apply(flip_matrix());
	for (auto& e : edges_) {
		e->flip();
	}
}

void tess::detail::tile_impl::set_parent(tess::patch_ptr parent, int index) {

	if (parent == nullptr)
		throw tess::error("invalid tile parent");
	parent_ = parent;
	index_ = index;
}

void tess::detail::tile_impl::detach()
{
	for (auto* v : vertices_)
		v->set_location(v->pos());
	parent_ = nullptr;
	index_ = -1;
}

bool tess::detail::tile_impl::has_parent() const {
	return parent_ != nullptr;
}

tess::const_patch_ptr tess::detail::tile_impl::parent() const {
	return parent_;
}

tess::patch_ptr tess::detail::tile_impl::parent()  {
	return parent_;
}

/*--------------------------------------------------------------------------------*/

tess::detail::edge_impl::edge_impl( obj_id id, tile_ptr parent, int index, int u, int v) :
	tessera_impl(id),
	parent_(parent),
	index_(index),
	u_(u),
	v_(v)
{}

tess::const_vertex_ptr tess::detail::edge_impl::u() const
{
	return parent_->vertex(u_);
}

tess::const_vertex_ptr tess::detail::edge_impl::v() const
{
	return parent_->vertex(v_);
}

tess::vertex_ptr tess::detail::edge_impl::u()
{
	return parent_->vertex(u_);
}

tess::vertex_ptr tess::detail::edge_impl::v()
{
	return parent_->vertex(v_);
}

tess::const_edge_ptr tess::detail::edge_impl::next_edge() const
{
	return v()->out_edge();
}

tess::const_edge_ptr tess::detail::edge_impl::prev_edge() const
{
	return u()->in_edge();
}


tess::value_ tess::detail::edge_impl::get_field(allocator& allocator, const std::string& field) const
{
	//TODO: do something about the raw strings here
	if (field == "next") {
		return value_(next_edge());
	}

	if (field == "prev") {
		return value_(prev_edge() );
	}

	if (field == "u") {
		return value_( u() );
	}

	if (field == "v") {
		return value_( v() );
	}

	return get_field(field);
}

tess::value_ tess::detail::edge_impl::get_field(const std::string& field) const
{
	if (fields_.find(field) != fields_.end())
		return fields_.at(field);

	return value_(nil_val());
}

void tess::detail::edge_impl::insert_field(const std::string& var, const value_& val)
{
	fields_[var] = val;
}

const std::map<std::string, tess::value_>& tess::edge::impl_type::fields() const
{
	return fields_;
}

bool tess::detail::edge_impl::has_property(const std::string& prop) const
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

tess::const_tile_ptr tess::detail::edge_impl::parent() const
{
	return parent_;
}

tess::tile_ptr tess::detail::edge_impl::parent()
{
	return parent_;
}

void  tess::edge::impl_type::flip()
{
	std::swap(u_, v_);
}

tess::edge_indices tess::detail::edge_impl::get_edge_location_indices() const {
	return edge_indices(
		u()->location_index(),
		v()->location_index()
	);
}


std::string tess::detail::edge_impl::debug() const
{
	std::stringstream ss;
	ss << "[ " << u_ << ":" << u()->debug()
		<< " -> " << v_ << ":" << v()->debug() << " ]";
	return ss.str();
}

void tess::detail::edge_impl::get_all_referenced_allocations(std::unordered_set<obj_id>& alloc_set) const
{
	auto key = get_id();
	if (alloc_set.find(key) != alloc_set.end())
		return;
	alloc_set.insert(key);

	parent_->get_all_referenced_allocations(alloc_set);
}


void  tess::detail::edge_impl::clone_to(tess::allocator& allocator, std::unordered_map<obj_id, void*>& orginal_to_clone, edge_ptr mutable_clone) const
{
	mutable_clone->index_ = index_;
	mutable_clone->u_ = u_;
	mutable_clone->v_ = v_;
	mutable_clone->parent_ = clone_object(allocator, orginal_to_clone, parent_);

	for (const auto& [var, val] : fields_) {
		mutable_clone->fields_[var] = tess::clone_value(allocator, orginal_to_clone, val);
	}
}

/*--------------------------------------------------------------------------------*/

tess::detail::vertex_impl::vertex_impl(obj_id id, const_tile_ptr parent, int n, std::tuple<number, number> loc) :
	tessera_impl(id),
	parent_(parent), index_(n), location_(loc)
{
}

std::tuple<double, double> tess::detail::vertex_impl::to_floats() const
{
	auto [x, y] = pos();
    return {
        static_cast<double>(x),
		static_cast<double>(y),
    };
}

tess::point tess::detail::vertex_impl::pos() const
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

tess::value_ tess::detail::vertex_impl::get_field(allocator& allocator, const std::string& field) const
{
	return {};
}

void tess::detail::vertex_impl::get_all_referenced_allocations(std::unordered_set<obj_id>& alloc_set) const
{
	auto key = get_id();
	if (alloc_set.find(key) != alloc_set.end())
		return;
	alloc_set.insert(key);

	parent_->get_all_referenced_allocations(alloc_set);
}

void tess::detail::vertex_impl::clone_to(tess::allocator& allocator, std::unordered_map<obj_id, void*>& orginal_to_clone, vertex_ptr mutable_clone) const
{
	mutable_clone->index_ = index_;
	mutable_clone->location_ = location_;
	mutable_clone->parent_ = clone_object(allocator, orginal_to_clone, parent_);
};

tess::const_patch_ptr tess::detail::vertex_impl::grandparent() const
{
	if (!parent_)
		return nullptr;
	return parent_->parent();
}

void tess::detail::vertex_impl::set_location(int n)
{
	location_ = n;
}

void tess::detail::vertex_impl::set_location(point pt)
{
	location_ = pt;
}

int tess::detail::vertex_impl::location_index() const
{
	return std::get<int>(location_);
}

std::string  tess::detail::vertex_impl::debug() const
{
	std::stringstream ss;
	auto [x, y] = pos();
	ss << "{" << index_ << " (" << x << " , " << y << " )} ";
	return ss.str();
}

void tess::detail::vertex_impl::apply(const tess::matrix& mat) {

	if (std::holds_alternative<point>(location_)) {
		location_ = apply_matrix(mat, std::get<point>(location_));
	} else {
		throw error("vertex::impl_type::apply called on patch vertex");
	}
}

tess::const_tile_ptr tess::detail::vertex_impl::parent() const
{
	return parent_;
}

//TODO: maybe make some lazy created table for doing this in sublinear time for tiles more than k sides?
tess::const_edge_ptr tess::detail::vertex_impl::in_edge() const
{
	auto edges = parent_->edges();
	auto iter = std::find_if(edges.begin(), edges.end(),
		[this](const auto* e) {
			return e->v() == this;
		}
	);
	return *iter;
}

tess::const_edge_ptr tess::detail::vertex_impl::out_edge() const
{
	auto edges = parent_->edges();
	auto iter = std::find_if(edges.begin(), edges.end(),
		[this](const auto* e) {
			return e->u() == this;
		}
	);
	return *iter;
}


