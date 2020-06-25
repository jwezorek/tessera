#include "tile_impl.h"
#include "cluster.h"
#include "parser/keywords.h"
#include "allocator.h"
#include <algorithm>

namespace {

	tess::cluster edges_as_cluster(tess::allocator& allocator,  const std::vector<tess::edge>& edges) {
		std::vector<tess::expr_value> cluster_contents(edges.size());
		std::transform(edges.begin(), edges.end(), cluster_contents.begin(),
			[](const tess::edge& e)->tess::expr_value { return { e }; }
		);
		return allocator.create<tess::cluster>( cluster_contents ); 
	}

	struct impl_cloner : public tess::tessera_impl
	{
		template<typename T>
		typename T::impl_type* clone(tess::allocator& allocator, std::unordered_map<void*, void*>& orginal_to_clone, typename T::impl_type* impl)
		{
			tess::expr_value wrapper = { make_tess_obj<T>(impl) };
			auto wrapper_clone = std::get<T>(wrapper.clone(allocator, orginal_to_clone));
			return  get_impl(wrapper_clone);
		}
	};
}

std::vector<std::tuple<tess::number, tess::number>> get_regular_poly_vert_loc(int n) {
	std::vector<std::tuple<tess::number, tess::number>> points(n);
	return points;
}

tess::tile::impl_type::impl_type(tess::allocator* allocator, const std::vector<std::tuple<tess::number, tess::number>>& vertex_locations, bool foo) :
	untouched_(true), parent_(nullptr)
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

const std::vector<tess::edge>& tess::tile::impl_type::edges() const
{
    return edges_;
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

void tess::tile::impl_type::get_all_referenced_allocations(std::unordered_set<void*>& alloc_set) const
{
	auto ptr = to_void_star(this);
	if (alloc_set.find(ptr) != alloc_set.end())
		return;
	alloc_set.insert(ptr);

	for (const auto& edge : edges_) 
		 expr_value{edge}.get_all_referenced_allocations(alloc_set);
	for (const auto& vertex : vertices_) 
		expr_value{vertex}.get_all_referenced_allocations(alloc_set);
	for (const auto& [var, val] : fields_)
		val.get_all_referenced_allocations(alloc_set);
}

void tess::tile::impl_type::clone_to(tess::allocator& allocator, std::unordered_map<void*, void*>& orginal_to_clone, tile::impl_type* clone) const
{
	clone->untouched_ = untouched_;
	for (const auto& v : vertices_) {
		clone->vertices_.push_back(std::get<vertex>(expr_value{ v }.clone(allocator, orginal_to_clone)));
	}
	for (const auto& e : edges_) {
		clone->edges_.push_back(std::get<edge>(expr_value{ e }.clone(allocator, orginal_to_clone)));
	}

	if (parent_ != nullptr) {
		impl_cloner cloner;
		clone->parent_ = cloner.clone<tile_patch>(allocator, orginal_to_clone, parent_);
	} else {
		clone->parent_ = nullptr;
	}

	for (const auto& [var, val] : fields_)
		clone->fields_[var] = val.clone(allocator, orginal_to_clone);
}
/*
void tess::tile::impl_type::debug()
{
	std::cout << "{ ";
	for (int i = 0; i < vertices_.size(); i++) {
		std::cout << "  vert " << i << " {";
		get_impl(vertices_[i])->debug();
		std::cout << "  } ";
	}
	std::cout << "}\n";
}
*/
tess::expr_value tess::tile::impl_type::get_field(allocator& allocator, const std::string& field) const
{
	if (fields_.find(field) != fields_.end())
		return fields_.at(field);

	if (field == parser::keyword(parser::kw::edge) || field == "edges") {
		return { edges_as_cluster(allocator, edges_) };
	}

	return { error(std::string("refrenced undefined tile edge, vertex, or field: ") + field ) };
}

bool tess::tile::impl_type::is_untouched() const
{
	return untouched_;
}

void tess::tile::impl_type::apply(const matrix& mat)
{
	for (auto& vertex : vertices_) {
		get_impl(vertex)->apply(mat);
	}
	untouched_ = false;
}

void tess::tile::impl_type::flip()
{
	auto untouched = untouched_;
	apply(flip_matrix());
	for (auto& e : edges_) {
		get_impl(e)->flip();
	}
	untouched_ = untouched;
}

void tess::tile::impl_type::set_parent(tess::tile_patch::impl_type* parent) {
	parent_ = parent;
	untouched_ = true;
}

bool tess::tile::impl_type::has_parent() const {
	return parent_ != nullptr;
}

tess::tile_patch::impl_type* tess::tile::impl_type::parent() const {
	return parent_;
}


/*--------------------------------------------------------------------------------*/

tess::edge::impl_type::impl_type( tile::impl_type* parent, int index, int u, int v) :
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
	return {};
}

tess::tile::impl_type* tess::edge::impl_type::parent() const
{
	return parent_;
}

void  tess::edge::impl_type::flip()
{
	std::swap(u_, v_);
}

void tess::edge::impl_type::get_all_referenced_allocations(std::unordered_set<void*>& alloc_set) const
{
	auto ptr = to_void_star(this);
	if (alloc_set.find(ptr) != alloc_set.end())
		return;
	alloc_set.insert(ptr);

	parent_->get_all_referenced_allocations(alloc_set);
}

void  tess::edge::impl_type::clone_to(tess::allocator& allocator, std::unordered_map<void*, void*>& orginal_to_clone, edge::impl_type* clone) const
{
	clone->index_ = index_;
	clone->u_ = u_;
	clone->v_ = v_;
	impl_cloner cloner;
	clone->parent_ = cloner.clone<tile>(allocator, orginal_to_clone, parent_);
}

/*--------------------------------------------------------------------------------*/

tess::vertex::impl_type::impl_type( tile::impl_type* parent, int n, std::tuple<number, number> loc) :
    parent_(parent), index_(n), x_(std::get<0>(loc)), y_(std::get<1>(loc))
{
}

std::tuple<double, double> tess::vertex::impl_type::to_floats() const
{
    return {
        static_cast<double>(x_),
		static_cast<double>(y_),
    };
}

tess::point tess::vertex::impl_type::pos() const
{
    return {
        x_,
        y_
    };
}

tess::expr_value tess::vertex::impl_type::get_field(allocator& allocator, const std::string& field) const
{
	return {};
}

void tess::vertex::impl_type::get_all_referenced_allocations(std::unordered_set<void*>& alloc_set) const
{
	auto ptr = to_void_star(this);
	if (alloc_set.find(ptr) != alloc_set.end())
		return;
	alloc_set.insert(ptr);

	parent_->get_all_referenced_allocations(alloc_set);
}

void tess::vertex::impl_type::clone_to(tess::allocator& allocator, std::unordered_map<void*, void*>& orginal_to_clone, vertex::impl_type* clone) const
{
	clone->index_ = index_;
	clone->x_ = x_;
	clone->y_ = y_;
	impl_cloner cloner;
	clone->parent_ = cloner.clone<tile>(allocator, orginal_to_clone, parent_);
};

/*
void tess::vertex::impl_type::debug()
{
	auto [x, y] = to_floats();
	std::cout << index_ << " (" << x << " , " << y << " ) ";
}
*/

void tess::vertex::impl_type::apply(const tess::matrix& mat) {

	auto [x,y] = apply_matrix( mat, pos() );
	x_ = x;
	y_ = y;
}

tess::tile::impl_type* tess::vertex::impl_type::parent() const
{
	return parent_;
}


