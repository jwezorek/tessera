#include "tile_impl.h"
#include "cluster.h"
#include "variant_util.h"
#include "parser/keywords.h"
#include "allocator.h"
#include <algorithm>
#include "ops.h"

namespace {

	tess::cluster_ptr edges_as_cluster(tess::allocator& allocator,  const std::vector<tess::edge_ptr>& edges) {
		std::vector<tess::expr_value> cluster_contents(edges.size());
		std::transform(edges.begin(), edges.end(), cluster_contents.begin(),
			[](tess::edge_ptr e)->tess::expr_value { return tess::expr_value(e); }
		);
		return allocator.create<tess::cluster_ptr>( cluster_contents );
	}

	template<typename T>
	T* perform_clone(tess::allocator& allocator, std::unordered_map<tess::obj_id, void*>& orginal_to_clone, T* impl)
	{
		tess::expr_value wrapper = tess::expr_value(impl);
		auto unwrapped_clone = std::get<const T*>(tess::clone_value(allocator, orginal_to_clone, wrapper));
		return const_cast<T*>(unwrapped_clone);
	}
}

std::vector<std::tuple<tess::number, tess::number>> get_regular_poly_vert_loc(int n) {
	std::vector<std::tuple<tess::number, tess::number>> points(n);
	return points;
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
		vertices_[i] = allocator->create<vertex_ptr>(this, i, vertex_locations[i]);
		edges_[i] = allocator->create<edge_ptr>(this, i, i, (i + 1) % n);
	}
}

const std::vector<tess::vertex_ptr>& tess::detail::tile_impl::vertices() const
{
    return vertices_;
}

std::vector<tess::vertex_ptr>& tess::detail::tile_impl::vertices()
{
	return vertices_;
}

const std::vector<tess::edge_ptr>& tess::detail::tile_impl::edges() const
{
    return edges_;
}

std::vector<tess::edge_ptr>& tess::detail::tile_impl::edges()
{
	return edges_;
}

void tess::detail::tile_impl::set( std::vector<tess::vertex_ptr>&& vertices, std::vector<tess::edge_ptr>&& edges )
{
	vertices_ = std::move(vertices);
    edges_ = std::move(edges);
}

void tess::detail::tile_impl::insert_field(const std::string& var, const expr_value& val)
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
		 tess::get_all_referenced_allocations(expr_value{ edge }, alloc_set);
	for (const auto& vertex : vertices_) 
		tess::get_all_referenced_allocations(expr_value{ vertex }, alloc_set);
	for (const auto& [var, val] : fields_)
		tess::get_all_referenced_allocations(val, alloc_set);
}

//TODO
void tess::detail::tile_impl::clone_to(tess::allocator& allocator, std::unordered_map<obj_id, void*>& orginal_to_clone, tile_ptr clone) const
{
	auto mutable_clone = const_cast<tile::impl_type*>(clone);
	for (const auto* v : vertices_) {
		auto v_clone = std::get<vertex_ptr>(tess::clone_value(allocator, orginal_to_clone, expr_value(v)));
		mutable_clone->vertices_.push_back(v_clone);
	}
	for (const auto& e : edges_) {
		auto e_clone = std::get<edge_ptr>( tess::clone_value(allocator, orginal_to_clone, expr_value(e)));
		mutable_clone->edges_.push_back(e_clone);
	}

	if (parent_ != nullptr) {
		mutable_clone->parent_ = perform_clone<tile_patch::impl_type>(allocator, orginal_to_clone, const_cast<tile_patch::impl_type*>(parent_));
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

template <typename T>
void* const_obj_ptr_to_void_star(const T* obj_ptr) {
	T* non_const_ptr = const_cast<T*>(obj_ptr);
	return reinterpret_cast<void*>(non_const_ptr);
}

//TODO
tess::tile_ptr tess::detail::tile_impl::clone_detached(tess::allocator& a) const
{
	if (is_detached())
		return tess::clone(a, this);
	
	// clone this tile such that its parent is only shallow copied.
	auto tile_value = expr_value( this );
	std::unordered_map<obj_id, void*> original_to_clone;
	auto this_patch_key =  parent_->get_id();
	original_to_clone[this_patch_key] = const_obj_ptr_to_void_star(parent_);
	expr_value clone_expr_value = tess::clone_value(a, original_to_clone, tile_value);

	//now return the clone with the parent detached.
	auto clone_tile = std::get<tess::tile_ptr>(clone_expr_value);
	const_cast<tile::impl_type*>(clone_tile)->detach();

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

const tess::tile_ptr tess::detail::tile_impl::get_adjacent_tile(int edge_index) const
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


tess::edge_ptr tess::detail::tile_impl::get_edge_on(tess::allocator& a,  edge_ptr edge) const
{
	if (!is_detached()) {
		return std::get<tess::edge_ptr>(parent_->get_on(a, edge));
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

tess::expr_value tess::detail::tile_impl::get_on(tess::allocator& a,  std::variant<tess::edge_ptr, tess::cluster_ptr>& var) const
{
	return std::visit(
		overloaded{
			[&]( tess::edge_ptr e) -> expr_value {
				auto maybe_edge = get_edge_on(a,e);
				if (maybe_edge) {
					return expr_value( maybe_edge );
				} else {
					return {};
				}
			},
			[&]( tess::cluster_ptr c) -> expr_value {
				const auto& items = c->items();
				std::vector<tess::expr_value> on_edges( items.size() );
				std::transform(items.begin(), items.end(), on_edges.begin(),
					[&](const expr_value& v) -> expr_value {
						std::variant<tess::edge_ptr, tess::cluster_ptr> var = variant_cast(v);
						return this->get_on(a, var);
					}
				);
				return expr_value(a.create<tess::cluster_ptr>(on_edges) );
			}
		},
		var
	);
}

tess::expr_value tess::detail::tile_impl::get_field(const std::string& field) const
{
	if (fields_.find(field) != fields_.end())
		return fields_.at(field);

	return expr_value(nil_val());
}

tess::expr_value tess::detail::tile_impl::get_field(allocator& allocator, const std::string& field) const
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

const std::map<std::string, tess::expr_value>& tess::detail::tile_impl::fields() const
{
	return fields_;
}

//TODO
void tess::detail::tile_impl::apply(const matrix& mat)
{
	for (auto* vertex : vertices_) {
		const_cast<vertex::impl_type*>(vertex)->apply(mat);
	}
}

//TODO
tess::tile_ptr tess::detail::tile_impl::flip(allocator& a) const
{
	tess::tile_ptr flippee;
	if (is_detached()) {
		flippee = tess::clone(a, this);
	} else {
		flippee = clone_detached(a);
	}
	const_cast<tile::impl_type*>(flippee)->flip();
	return flippee;
}

//TODO
void tess::detail::tile_impl::flip()
{
	apply(flip_matrix());
	for (auto& e : edges_) {
		const_cast<edge::impl_type*>(e)->flip();
	}
}

void tess::detail::tile_impl::set_parent(tess::patch_ptr parent, int index) {

	if (parent == nullptr)
		throw tess::error("invalid tile parent");
	parent_ = parent;
	index_ = index;
}

//TODO
void tess::detail::tile_impl::detach()
{
	for (auto* v : vertices_)
		const_cast<vertex::impl_type*>(v)->set_location(v->pos());
	parent_ = nullptr;
	index_ = -1;
}

bool tess::detail::tile_impl::has_parent() const {
	return parent_ != nullptr;
}

tess::patch_ptr tess::detail::tile_impl::parent() const {
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

tess::vertex_ptr tess::detail::edge_impl::u() const
{
    return parent_->vertices().at(u_);
}

tess::vertex_ptr tess::detail::edge_impl::v() const
{
    return parent_->vertices().at(v_);
}

tess::vertex_ptr tess::detail::edge_impl::u()
{
	return parent_->vertices().at(u_);
}

tess::vertex_ptr tess::detail::edge_impl::v()
{
	return parent_->vertices().at(v_);
}

tess::edge_ptr tess::detail::edge_impl::next_edge() const
{
	return v()->out_edge();
}

tess::edge_ptr tess::detail::edge_impl::prev_edge() const
{
	return u()->in_edge();
}

tess::expr_value tess::detail::edge_impl::get_field(allocator& allocator, const std::string& field) const
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

tess::expr_value tess::detail::edge_impl::get_field(const std::string& field) const
{
	if (fields_.find(field) != fields_.end())
		return fields_.at(field);

	return expr_value(nil_val());
}

void tess::detail::edge_impl::insert_field(const std::string& var, const expr_value& val)
{
	fields_[var] = val;
}

const std::map<std::string, tess::expr_value>& tess::edge::impl_type::fields() const
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

tess::tile_ptr tess::detail::edge_impl::parent() const
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

//TODO
void  tess::detail::edge_impl::clone_to(tess::allocator& allocator, std::unordered_map<obj_id, void*>& orginal_to_clone, edge_ptr clone) const
{
	auto mutable_clone = const_cast<edge::impl_type*>(clone);
	mutable_clone->index_ = index_;
	mutable_clone->u_ = u_;
	mutable_clone->v_ = v_;
	mutable_clone->parent_ = perform_clone<tile::impl_type>(allocator, orginal_to_clone, const_cast<tile::impl_type*>(parent_));

	for (const auto& [var, val] : fields_) {
		mutable_clone->fields_[var] = tess::clone_value(allocator, orginal_to_clone, val);
	}
}

/*--------------------------------------------------------------------------------*/

tess::detail::vertex_impl::vertex_impl(obj_id id, tile_ptr parent, int n, std::tuple<number, number> loc) :
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

tess::expr_value tess::detail::vertex_impl::get_field(allocator& allocator, const std::string& field) const
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

//TODO
void tess::detail::vertex_impl::clone_to(tess::allocator& allocator, std::unordered_map<obj_id, void*>& orginal_to_clone, vertex_ptr clone) const
{
	auto mutable_clone = const_cast<vertex::impl_type*>(clone);
	mutable_clone->index_ = index_;
	mutable_clone->location_ = location_;
	mutable_clone->parent_ = perform_clone<tile::impl_type>(allocator, orginal_to_clone, const_cast<tile::impl_type*>(parent_));
};

tess::patch_ptr tess::detail::vertex_impl::grandparent() const
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

tess::tile_ptr tess::detail::vertex_impl::parent() const
{
	return parent_;
}

//TODO: maybe make some lazy created table for doing this in sublinear time for tiles more than k sides?
tess::edge_ptr tess::detail::vertex_impl::in_edge() const
{
	auto& edges = parent_->edges();
	auto iter = std::find_if(edges.begin(), edges.end(),
		[this](const auto* e) {
			return e->v() == this;
		}
	);
	return *iter;
}

tess::edge_ptr tess::detail::vertex_impl::out_edge() const
{
	auto& edges = parent_->edges();
	auto iter = std::find_if(edges.begin(), edges.end(),
		[this](const auto* e) {
			return e->u() == this;
		}
	);
	return *iter;
}


