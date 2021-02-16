#pragma once

#include "tessera_impl.h"
#include "number.h"
#include "variant_util.h"
#include <variant>
#include <any>
#include <memory>
#include <unordered_set>
#include <unordered_map>
#include "gc_heap.h"

namespace tess {

	template<typename T>
	tess::graph_root_ptr<const T> to_const(const tess::graph_root_ptr<T>& p) {
		return tess::graph_pool::const_pointer_cast<const T>(p);
	}

	template<typename T>
	tess::graph_root_ptr<T> from_const(const tess::graph_root_ptr<const T>& p) {
		return tess::graph_pool::const_pointer_cast<T>(p);
	}

	namespace detail {
		class vertex_impl;
		class edge_impl;
		class cluster_impl;
		class tile_impl;
		class patch_impl;
		class lambda_impl;

		template<typename T>
		tess::graph_root_ptr<T> clone_aux(tess::gc_heap& allocator, std::unordered_map<tess::obj_id, std::any>& orginal_to_clone, const tess::graph_root_ptr<const T>& original) {

			auto key = original->get_id();
			tess::graph_root_ptr<T>  clone_impl = {};

			if (orginal_to_clone.find(key) != orginal_to_clone.end()) {
				clone_impl = std::any_cast<tess::graph_root_ptr<T>>(orginal_to_clone[key]);
			} else {
				clone_impl = allocator.make_blank<tess::graph_root_ptr<const T>>();
				orginal_to_clone[key] = clone_impl;
				original->clone_to(allocator, orginal_to_clone, clone_impl.get());
			}

			return clone_impl;
		};

		template<typename T, typename U>
		tess::graph_ptr<T> clone_aux(const U& u, tess::gc_heap& allocator, std::unordered_map<tess::obj_id, std::any>& orginal_to_clone, const tess::graph_ptr<const T>& original) {
			tess::graph_root_ptr<const T> val(original);
			auto clone = clone_aux(allocator, orginal_to_clone, val);
			return tess::graph_ptr<T>(u, clone);
		};

	}

	class serialization_state {
	public:
		serialization_state();
		std::optional<tess::obj_id> get_obj(tess::obj_id id) const;
		tess::obj_id insert(tess::obj_id id);
	private:
		std::unordered_map<obj_id, tess::obj_id> tbl_;
		tess::obj_id current_id_;
	};

	class execution_state;

	class nil_val {
	public:
		nil_val();
	};

	bool operator==(nil_val lhs, nil_val rhs);

	class field_ref_impl;
	using field_ref_ptr = std::shared_ptr<field_ref_impl>;

	using vertex_raw_ptr = detail::vertex_impl*;
	using edge_raw_ptr = detail::edge_impl*;
	using cluster_raw_ptr = detail::cluster_impl*;
	using tile_raw_ptr = detail::tile_impl*;
	using patch_raw_ptr = detail::patch_impl*;
	using lambda_raw_ptr = detail::lambda_impl*;

	using const_vertex_root_ptr = graph_root_ptr<const detail::vertex_impl>;
	using const_edge_root_ptr = graph_root_ptr<const detail::edge_impl>;
	using const_cluster_root_ptr = graph_root_ptr<const detail::cluster_impl>;
	using const_tile_root_ptr = graph_root_ptr<const detail::tile_impl>;
	using const_patch_root_ptr = graph_root_ptr<const detail::patch_impl>;
	using const_lambda_root_ptr = graph_root_ptr<const detail::lambda_impl>;

	using vertex_root_ptr = graph_root_ptr<detail::vertex_impl>;
	using edge_root_ptr = graph_root_ptr<detail::edge_impl>;
	using cluster_root_ptr = graph_root_ptr<detail::cluster_impl>;
	using tile_root_ptr = graph_root_ptr<detail::tile_impl>;
	using patch_root_ptr = graph_root_ptr<detail::patch_impl>;
	using lambda_root_ptr = graph_root_ptr<detail::lambda_impl>;

	using vertex_graph_ptr = graph_ptr<detail::vertex_impl>;
	using edge_graph_ptr = graph_ptr<detail::edge_impl>;
	using cluster_graph_ptr = graph_ptr<detail::cluster_impl>;
	using tile_graph_ptr = graph_ptr<detail::tile_impl>;
	using patch_graph_ptr = graph_ptr<detail::patch_impl>;
	using lambda_graph_ptr = graph_ptr<detail::lambda_impl>;

	using value_ = std::variant<const_tile_root_ptr, const_patch_root_ptr, const_edge_root_ptr, const_vertex_root_ptr, const_lambda_root_ptr, const_cluster_root_ptr, field_ref_ptr, nil_val, number, std::string, bool>;
	using field_value = std::variant<tile_graph_ptr, patch_graph_ptr, edge_graph_ptr, vertex_graph_ptr, lambda_graph_ptr, cluster_graph_ptr, nil_val, number, std::string, bool>;

	template<typename U>
	field_value to_field_value(const tess::graph_ptr<U>& u, const value_& v)
	{
		if (is_simple_value(v))
			return variant_cast(v);
		std::variant<const_tile_root_ptr, const_patch_root_ptr, const_vertex_root_ptr, const_edge_root_ptr, const_cluster_root_ptr, const_lambda_root_ptr> obj_variant = variant_cast(v);
		return std::visit(
			[&u](auto ptr)->field_value {
				using V = typename decltype(ptr)::value_type;
				return tess::field_value( tess::graph_ptr<std::remove_const_t<V>>(u, from_const(ptr)) );
			},
			obj_variant
		);
	}

	value_ from_field_value(const field_value& fv);

	template<typename T>
	graph_root_ptr<T> to_root_ptr(const graph_ptr<T>& gptr) {
		return { }; // TODO:GP
	}

	template<typename T>
	graph_ptr<T>&& to_graph_ptr( const graph_root_ptr<T>& ptr) {
		return{}; // TODO:GP
	}

	template <typename T>
	struct value_traits {
		using obj_variant = void;

		template<typename U>
		using ptr_type = void;
	};

	template <>
	struct value_traits<value_> {
		using obj_variant = std::variant<const_tile_root_ptr, const_patch_root_ptr, const_edge_root_ptr, const_vertex_root_ptr, const_lambda_root_ptr, const_cluster_root_ptr>;

		template<typename U>
		using ptr_type = graph_root_ptr<U>;
	};

	template <>
	struct value_traits<field_value> {
		using obj_variant = std::variant<tile_graph_ptr, patch_graph_ptr, edge_graph_ptr,vertex_graph_ptr, lambda_graph_ptr, cluster_graph_ptr>;

		template<typename U>
		using ptr_type = graph_ptr<U>;
	};

	template <typename V>
	bool is_simple_value(const V& v) {
		return std::holds_alternative<nil_val>(v) ||
			std::holds_alternative<number>(v) ||
			std::holds_alternative<std::string>(v) ||
			std::holds_alternative<bool>(v);
	};

	template <typename V>
	bool is_object_like(const V& v) {
		// by "object-like" we mean epression values that may have fields.
		return std::holds_alternative<const_tile_root_ptr>(v) ||
			std::holds_alternative<const_patch_root_ptr>(v) ||
			std::holds_alternative<const_vertex_root_ptr>(v) ||
			std::holds_alternative<const_edge_root_ptr>(v) ||
			std::holds_alternative<const_cluster_root_ptr>(v) ||
			std::holds_alternative<const_lambda_root_ptr>(v);
	}

	template <typename V>
	bool is_array_like(const V& v) {
		// by "array-like" we mean epression values that may be dereferenced via the [] operator.
		return  std::holds_alternative<const_patch_root_ptr>(v) ||
			std::holds_alternative<const_cluster_root_ptr>(v);
	}

	template <typename V>
	bool is_nil(const V& v) {
		return std::holds_alternative<tess::nil_val>(v);
	}

	value_ clone_value(gc_heap& allocator, std::unordered_map<tess::obj_id, std::any>& original_to_clone, const value_& v);
	value_ clone_value(gc_heap& allocator, const value_& v);

	template<typename U>
	field_value clone_value(const tess::graph_ptr<U>& u, gc_heap& a,  std::unordered_map<tess::obj_id, std::any>& original_to_clone, const field_value& v) {
		auto root_ptr_val = from_field_value(v);
		auto clone = clone_value(a, original_to_clone, root_ptr_val);
		return to_field_value(u, clone);
	}

	template <typename U>
	field_value clone_value(const tess::graph_ptr<U>& u, gc_heap& allocator,  const field_value& v) {
		std::unordered_map<obj_id, std::any> original_to_clone;
		return clone_value(allocator, u, original_to_clone, v);
	}

	field_value&& copy_field(const field_value& fv);

	value_ get_ary_item(value_ v, int index);
	int get_ary_count(value_ v);
	value_ get_field(value_ v, gc_heap& allocator, const std::string& field) ;
	void insert_field(value_ v, const std::string& var, value_ val);
	std::string to_string(value_ v);

	std::string serialize(value_ v);
	std::string serialize(serialization_state& state, value_ v);

	bool operator==(const value_& lhs, const value_& rhs);
	bool operator!=(const value_& lhs, const value_& rhs);

	bool operator==(const field_value&lhs, const field_value& rhs);
	bool operator!=(const field_value& lhs, const field_value& rhs);

	template<typename T>
	value_ make_value(const graph_root_ptr<T>& v) {
		return { tess::to_const(v) };
	}

	template<typename T>
	field_value&& make_value(graph_ptr<T>&& v) {
		return { std::move(v) };
	}

	template<typename T>
	auto get_mutable(value_ val) {
		if constexpr ((std::is_same<T, field_ref_ptr>::value) || (std::is_same<T, nil_val>::value) || (std::is_same<T, number>::value) ||
			(std::is_same<T, std::string>::value) || (std::is_same<T, bool>::value)) {
			return std::get<T>(val);
		} else {
			auto c_val = std::get<T>(val);
			using base_type = typename decltype(c_val)::value_type;
			using mutable_type = std::remove_const_t<base_type>;
			return graph_pool::const_pointer_cast<mutable_type>(c_val);
		}
	}

	template<typename T>
	auto clone_object(tess::gc_heap& allocator, std::unordered_map<tess::obj_id, std::any>& orginal_to_clone, const graph_root_ptr<T>& impl)
	{
		value_ wrapper = to_const(impl);
		auto clone = tess::get_mutable<graph_root_ptr<T>>(tess::clone_value(allocator, orginal_to_clone, wrapper));
		return clone;
	}

	template<typename T>
	auto clone_object(tess::gc_heap& a, const graph_root_ptr<T>& impl)
	{
		std::unordered_map<tess::obj_id, std::any> orginal_to_clone;
		return clone_object(a, orginal_to_clone, impl);
	}

	template<typename T, typename U>
	graph_ptr<T> clone_object(const U& u, tess::gc_heap& a, std::unordered_map<tess::obj_id, std::any>& orginal_to_clone, const graph_ptr<T>& impl)
	{
		auto root_ptr = to_const(to_root_ptr(impl));
		auto clone = clone_object(a, orginal_to_clone, root_ptr);
		return graph_ptr<T>(u, clone);
	}

	template<typename T, typename U>
	graph_ptr<T> clone_object(const U& u, tess::gc_heap& a, const graph_ptr<T>& impl)
	{
		std::unordered_map<tess::obj_id, std::any> orginal_to_clone;
		return clone_object(u, a, orginal_to_clone, impl);
	}
}