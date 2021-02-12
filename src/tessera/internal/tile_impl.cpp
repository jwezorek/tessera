#include "tile_impl.h"
#include "lambda_impl.h"
#include "cluster.h"
#include "variant_util.h"
#include "parser/keywords.h"
#include "gc_heap.h"
#include <algorithm>
#include "ops.h"

namespace {

    template<typename I>
	tess::const_cluster_root_ptr edges_as_cluster(tess::gc_heap& allocator,  I begin_edges, I end_edges) {
		std::vector<tess::value_> cluster_contents(std::distance(begin_edges, end_edges));
		std::transform(begin_edges, end_edges, cluster_contents.begin(),
			[](auto e)->tess::value_ { return tess::value_(to_const(to_root_ptr(e))); }
		);
		return allocator.make_const<tess::const_cluster_root_ptr>( cluster_contents );
	}

	template<typename T>
	auto clone_object(tess::gc_heap& allocator, std::unordered_map<tess::obj_id, std::any>& orginal_to_clone, T impl)
	{
		auto wrapper = tess::make_value(impl);

		using base_type = typename T::value_type;
		using ptr_t = typename tess::value_traits<decltype(wrapper)>::ptr_type<const base_type>;

		auto clone = tess::clone_value(allocator, orginal_to_clone, wrapper);
		return tess::get_mutable<ptr_t>(clone);
	}

	template<typename T>
	auto clone_object(tess::gc_heap& a, T impl)
	{
		std::unordered_map<tess::obj_id, std::any> orginal_to_clone;
		return clone_object(a, orginal_to_clone, impl);
	}

}

tess::detail::tile_impl::tile_impl( tess::gc_heap& a, const std::vector<std::tuple<tess::number, tess::number>>& vertex_locations) :
	parent_(nullptr),
	index_(-1)
{
	auto n = static_cast<int>(vertex_locations.size());
	vertices_.resize(n);
	edges_.resize(n);

	for (int i = 0; i < n; ++i) {
		vertices_[i] = a.make_mutable<vertex_root_ptr>( i, vertex_locations[i]);
		edges_[i] = a.make_mutable<edge_root_ptr>( i, i, (i + 1) % n);
	}
}

void tess::detail::tile_impl::initialize( tile_root_ptr self)
{
    self_ = self;
    for (auto v : vertices_)
        v->set_parent( const_tile_root_ptr(self) );
    for (auto e : edges_)
        e->set_parent( const_tile_root_ptr(self) );
}

tess::detail::tile_impl::const_vertex_iter tess::detail::tile_impl::begin_vertices() const {
	return vertices_.begin();
}

tess::detail::tile_impl::const_vertex_iter tess::detail::tile_impl::end_vertices() const {
	return vertices_.end();
}

tess::detail::tile_impl::const_edge_iter tess::detail::tile_impl::begin_edges() const {
	return edges_.begin();
}

tess::detail::tile_impl::const_edge_iter tess::detail::tile_impl::end_edges() const {
	return edges_.end();
}

tess::detail::tile_impl::vertex_iter tess::detail::tile_impl::begin_vertices() {
	return vertices_.begin();
}

tess::detail::tile_impl::vertex_iter tess::detail::tile_impl::end_vertices() {
	return vertices_.end();
}

tess::detail::tile_impl::edge_iter tess::detail::tile_impl::begin_edges() {
	return edges_.begin();
}

tess::detail::tile_impl::edge_iter tess::detail::tile_impl::end_edges() {
	return edges_.end();
}

tess::const_vertex_root_ptr tess::detail::tile_impl::vertex(int index) const
{
	return to_root_ptr( vertices_.at(index) );
}

tess::vertex_root_ptr tess::detail::tile_impl::vertex(int index)
{
	return to_root_ptr(vertices_.at(index));
}

tess::const_edge_root_ptr tess::detail::tile_impl::edge(int index) const
{
	return to_root_ptr(edges_.at(index));
}

tess::edge_root_ptr tess::detail::tile_impl::edge(int index)
{
	return to_root_ptr(edges_.at(index));
}

void tess::detail::tile_impl::insert_field(const std::string& var, const value_& val)
{
	fields_[var] = variant_cast(val);
}

void tess::detail::tile_impl::clone_to(tess::gc_heap& allocator, std::unordered_map<obj_id, std::any>& orginal_to_clone, tile_raw_ptr mutable_clone) const
{
	for (auto v : vertices_) { // clone vertices
		mutable_clone->vertices_.push_back( clone_object(allocator, orginal_to_clone, v) );
	}
	for (auto e : edges_) { // clone edges
		mutable_clone->edges_.push_back( clone_object(allocator, orginal_to_clone, e) );
	}

	if (parent_) { // clone parent
		mutable_clone->parent_ = clone_object(allocator, orginal_to_clone, parent_);
	} else {
		mutable_clone->parent_ = {};
	}

	for (const auto& [var, val] : fields_) { //clone fields
		mutable_clone->fields_[var] = tess::clone_value(allocator, orginal_to_clone, val);
	}
}

bool tess::detail::tile_impl::is_detached() const
{
	return parent_ == patch_graph_ptr{};
}


tess::tile_root_ptr tess::detail::tile_impl::clone_detached(tess::gc_heap& a) const
{
	if (is_detached())
		return tess::to_root_ptr(clone_object(a, self_));
	
	// clone this tile such that its parent is only shallow copied.
	auto tile_value = value_( to_const(to_root_ptr(self_)) );
	std::unordered_map<obj_id, std::any> original_to_clone;
	auto this_patch_key =  parent_->get_id();
	original_to_clone[this_patch_key] = to_root_ptr(parent_);
	value_ clone_value = tess::clone_value(a, original_to_clone, tile_value);

	//now return the clone with the parent detached.
	auto clone_tile = get_mutable<tess::const_tile_root_ptr>(clone_value);
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

tess::const_tile_root_ptr tess::detail::tile_impl::get_adjacent_tile(int edge_index) const
{
	if (is_detached())
		return nullptr;

	auto e = edges_[edge_index];
	int u = e->u()->location_index();
	int v = e->v()->location_index();

	auto adj_edge = parent_->get_edge_on(v, u);
	return (adj_edge) ? adj_edge->parent() : nullptr;
}

tess::const_edge_root_ptr tess::detail::tile_impl::get_edge_on(tess::gc_heap& a,  const_edge_root_ptr edge) const
{
	if (!is_detached()) {
		return std::get<tess::const_edge_root_ptr>(parent_->get_on(a, edge));
	}

	//TODO: use an rtree here.
	tess::point u = edge->u()->pos();
	tess::point v = edge->v()->pos();

	static auto eps = static_cast<tess::number>(std::numeric_limits<float>::epsilon());

	for (auto iter = begin_edges(); iter != end_edges(); ++iter) {
		auto e = *iter;
		auto e_u = e->u()->pos();
		auto e_v = e->v()->pos();

		if (tess::distance(e_u, u) <= eps && tess::distance(e_v, v) <= eps)
			return to_root_ptr(e);
	}

	return nullptr;;
}

tess::value_ tess::detail::tile_impl::get_on(tess::gc_heap& a,  std::variant<tess::const_edge_root_ptr, tess::const_cluster_root_ptr>& var) const
{
	return std::visit(
		overloaded{
			[&]( tess::const_edge_root_ptr e) -> value_ {
				auto maybe_edge = get_edge_on(a,e);
				if (maybe_edge) {
					return value_( maybe_edge );
				} else {
					return {};
				}
			},
			[&]( tess::const_cluster_root_ptr c) -> value_ {
				const auto& items = c->items();
				std::vector<tess::value_> on_edges( items.size() );
				std::transform(items.begin(), items.end(), on_edges.begin(),
					[&](const value_& v) -> value_ {
						std::variant<tess::const_edge_root_ptr, tess::const_cluster_root_ptr> var = variant_cast(v);
						return this->get_on(a, var);
					}
				);
				return value_( a.make_const<tess::const_cluster_root_ptr>( on_edges) );
			}
		},
		var
	);
}

tess::value_ tess::detail::tile_impl::get_field(const std::string& field) const
{
	if (fields_.find(field) != fields_.end())
		return from_field_value(fields_.at(field));

	return value_(nil_val());
}

tess::value_ tess::detail::tile_impl::get_field(gc_heap& allocator, const std::string& field) const
{
	if (field == parser::keyword(parser::kw::edges)) {
		return value_(edges_as_cluster(allocator, edges_.begin(), edges_.end()) );
	}

	auto val = get_field(field);
	if (!std::holds_alternative< nil_val>(val))
		return val;
	else
		throw tess::error(std::string("refrenced undefined tile edge, vertex, or field: ") + field );
}

const std::map<std::string, tess::field_value>& tess::detail::tile_impl::fields() const
{
	return fields_;
}

void tess::detail::tile_impl::apply(const matrix& mat)
{
	for (auto vertex : vertices_) {
		vertex->apply(mat);
	}
}

tess::const_tile_root_ptr tess::detail::tile_impl::flip(gc_heap& a) const
{
	tess::tile_root_ptr flippee;
	if (is_detached()) {
		flippee = to_root_ptr(clone_object(a, self_));
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

void tess::detail::tile_impl::set_parent(tess::patch_root_ptr parent, int index) {

	if (parent == nullptr)
		throw tess::error("invalid tile parent");
	parent_ = parent;
	index_ = index;
}

void tess::detail::tile_impl::detach()
{
	for (auto v : vertices_)
		v->set_location(v->pos());
	parent_ = {};
	index_ = -1;
}

bool tess::detail::tile_impl::has_parent() const {
	return parent_ != tess::patch_graph_ptr{};
}

tess::const_patch_root_ptr tess::detail::tile_impl::parent() const {
	return to_root_ptr(parent_);
}

tess::patch_root_ptr tess::detail::tile_impl::parent()  {
	return to_root_ptr(parent_);
}

/*--------------------------------------------------------------------------------*/

tess::detail::edge_impl::edge_impl( gc_heap& a, int index, int u, int v) :
	parent_(nullptr),
	index_(index),
	u_(u),
	v_(v)
{
}

void tess::detail::edge_impl::set_parent(tess::const_tile_root_ptr parent)
{
    parent_ = to_graph_ptr(parent);
}

tess::const_vertex_root_ptr tess::detail::edge_impl::u() const
{
	return parent_->vertex(u_);
}

tess::const_vertex_root_ptr tess::detail::edge_impl::v() const
{
	return parent_->vertex(v_);
}

tess::vertex_root_ptr tess::detail::edge_impl::u()
{
	return parent_->vertex(u_);
}

tess::vertex_root_ptr tess::detail::edge_impl::v()
{
	return parent_->vertex(v_);
}

tess::const_edge_root_ptr tess::detail::edge_impl::next_edge() const
{
	return v()->out_edge();
}

tess::const_edge_root_ptr tess::detail::edge_impl::prev_edge() const
{
	return u()->in_edge();
}


tess::value_ tess::detail::edge_impl::get_field(gc_heap& allocator, const std::string& field) const
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
		return variant_cast(fields_.at(field));

	return value_(nil_val());
}

void tess::detail::edge_impl::insert_field(const std::string& var, const value_& val)
{
	fields_[var] = variant_cast(val);
}

const std::map<std::string, tess::field_value>& tess::edge::impl_type::fields() const
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

tess::const_tile_root_ptr tess::detail::edge_impl::parent() const
{
	return to_root_ptr(parent_);
}

tess::tile_root_ptr tess::detail::edge_impl::parent()
{
	return to_root_ptr(parent_);
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


void  tess::detail::edge_impl::clone_to(tess::gc_heap& allocator, std::unordered_map<obj_id, std::any>& orginal_to_clone, edge_raw_ptr mutable_clone) const
{
	mutable_clone->index_ = index_;
	mutable_clone->u_ = u_;
	mutable_clone->v_ = v_;

	if (parent_)
	    mutable_clone->parent_ = clone_object(allocator, orginal_to_clone, parent_); // clone parent
	else
		mutable_clone->parent_ = {};

	for (const auto& [var, val] : fields_) {
		mutable_clone->fields_[var] = tess::clone_value(allocator, orginal_to_clone, val); // clone fields
	}
}

/*--------------------------------------------------------------------------------*/

tess::detail::vertex_impl::vertex_impl( gc_heap& a, int n, std::tuple<number, number> loc) :
	parent_(nullptr), index_(n), location_(loc)
{
}

void tess::detail::vertex_impl::set_parent(const_tile_root_ptr parent)
{
    parent_ = parent;
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
				auto patch = this->grandparent();
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

tess::value_ tess::detail::vertex_impl::get_field(gc_heap& allocator, const std::string& field) const
{
	return {}; //TODO
}


void tess::detail::vertex_impl::clone_to(tess::gc_heap& allocator, std::unordered_map<obj_id, std::any>& orginal_to_clone, vertex_raw_ptr mutable_clone) const
{
	mutable_clone->index_ = index_;
	mutable_clone->location_ = location_;
	if (parent_) {
		tess::g_ptr<detail::tile_impl> parent_clone = clone_object(allocator, orginal_to_clone, parent_); // parent
		mutable_clone->parent_ = to_const(parent_clone);
	} else {
		mutable_clone->parent_ = {};
	}
};

tess::const_patch_root_ptr tess::detail::vertex_impl::grandparent() const
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

tess::const_tile_root_ptr tess::detail::vertex_impl::parent() const
{
	return to_root_ptr(parent_);
}

//TODO: maybe make some lazy created table for doing this in sublinear time for tiles more than k sides?
tess::const_edge_root_ptr tess::detail::vertex_impl::in_edge() const
{
	auto iter = std::find_if(parent_->begin_edges(), parent_->end_edges(),
		[this]( auto e) {
			return e->v().get() == this;
		}
	);
	return to_root_ptr(*iter);
}

tess::const_edge_root_ptr tess::detail::vertex_impl::out_edge() const
{
	auto iter = std::find_if(parent_->begin_edges(), parent_->end_edges(),
		[this]( auto e) {
			return e->u().get() == this;
		}
	);
	return to_root_ptr(*iter);
}


