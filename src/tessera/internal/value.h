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
	struct g_ptr {
		using value_type = T;

		g_ptr() {}
		g_ptr(const gcpp::deferred_ptr<T>& p) : obj(p) {
		}

		template <typename U>
		g_ptr( g_ptr<const U> u) {
			obj = gcpp::deferred_ptr<U>(u.obj);
		}

		explicit operator bool() const { return obj.get(); }
		auto operator->() { return obj.operator->(); }
		const auto operator->() const { return obj.operator->(); }
		gcpp::deferred_ptr<T> obj;
	};

	template<typename T>
	g_ptr<const T> to_const(g_ptr<T> p) {
		return g_ptr<const T>(p.obj);
	}

	template<typename T>
	gcpp::deferred_ptr<const T> to_const(gcpp::deferred_ptr<T> p) {
		return gcpp::deferred_ptr<const T>(p);
	}

	template<typename T>
	bool operator==(const g_ptr<T>& lhs, const g_ptr<T>& rhs) {
		return lhs.obj == rhs.obj;
	}

	template<typename T>
	bool operator!=(const g_ptr<T>& lhs, const g_ptr<T>& rhs) {
		return lhs.obj != rhs.obj;
	}

	namespace detail {
		class vertex_impl;
		class edge_impl;
		class cluster_impl;
		class tile_impl;
		class patch_impl;
		class lambda_impl;

		template<typename T>
		gcpp::deferred_ptr<T> clone_aux(tess::gc_heap& allocator, std::unordered_map<tess::obj_id, std::any>& orginal_to_clone, gcpp::deferred_ptr<const T> original) {

			auto key = original->get_id();
			gcpp::deferred_ptr<T>  clone_impl = nullptr;

			if (orginal_to_clone.find(key) != orginal_to_clone.end()) {
				clone_impl = std::any_cast<gcpp::deferred_ptr<T>>(orginal_to_clone[key]);
			}
			else {
				clone_impl = allocator.make_mutable<gcpp::deferred_ptr<const T>>();
				orginal_to_clone[key] = clone_impl;
				original->clone_to(allocator, orginal_to_clone, clone_impl.get());
			}

			return clone_impl;
		};

		template<typename T>
		tess::g_ptr<T> clone_aux(tess::gc_heap& allocator, std::unordered_map<tess::obj_id, std::any>& orginal_to_clone, tess::g_ptr<T> original) {
			gcpp::deferred_ptr<T> val = original.obj;
			auto clone = clone_aux(allocator, orginal_to_clone, val);
			return tess::g_ptr<T>(clone);
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

	using const_vertex_root_ptr = gcpp::deferred_ptr<const detail::vertex_impl>;
	using const_edge_root_ptr = gcpp::deferred_ptr<const detail::edge_impl>;
	using const_cluster_root_ptr = gcpp::deferred_ptr<const detail::cluster_impl>;
	using const_tile_root_ptr = gcpp::deferred_ptr<const detail::tile_impl>;
	using const_patch_root_ptr = gcpp::deferred_ptr<const detail::patch_impl>;
	using const_lambda_root_ptr = gcpp::deferred_ptr<const detail::lambda_impl>;

	using vertex_root_ptr = gcpp::deferred_ptr<detail::vertex_impl>;
	using edge_root_ptr = gcpp::deferred_ptr<detail::edge_impl>;
	using cluster_root_ptr = gcpp::deferred_ptr<detail::cluster_impl>;
	using tile_root_ptr = gcpp::deferred_ptr<detail::tile_impl>;
	using patch_root_ptr = gcpp::deferred_ptr<detail::patch_impl>;
	using lambda_root_ptr = gcpp::deferred_ptr<detail::lambda_impl>;

	using const_vertex_graph_ptr = g_ptr<const detail::vertex_impl>;
	using const_edge_graph_ptr = g_ptr<const detail::edge_impl>;
	using const_cluster_graph_ptr = g_ptr<const detail::cluster_impl>;
	using const_tile_graph_ptr = g_ptr<const detail::tile_impl>;
	using const_patch_graph_ptr = g_ptr<const detail::patch_impl>;
	using const_lambda_graph_ptr = g_ptr<const detail::lambda_impl>;

	using vertex_graph_ptr = g_ptr<detail::vertex_impl>;
	using edge_graph_ptr = g_ptr<detail::edge_impl>;
	using cluster_graph_ptr = g_ptr<detail::cluster_impl>;
	using tile_graph_ptr = g_ptr<detail::tile_impl>;
	using patch_graph_ptr = g_ptr<detail::patch_impl>;
	using lambda_graph_ptr = g_ptr<detail::lambda_impl>;

	using value_ = std::variant<const_tile_root_ptr, const_patch_root_ptr, const_edge_root_ptr, const_vertex_root_ptr, const_lambda_root_ptr, const_cluster_root_ptr, field_ref_ptr, nil_val, number, std::string, bool>;
	using field_value = std::variant<const_tile_graph_ptr, const_patch_graph_ptr, const_edge_graph_ptr, const_vertex_graph_ptr, const_lambda_graph_ptr, const_cluster_graph_ptr, nil_val, number, std::string, bool>;

	field_value to_field_value(const value_& v);
	value_ from_field_value(const field_value& fv);

	template<typename T>
	gcpp::deferred_ptr<T> to_root_ptr(g_ptr<T> gptr) {
		return gptr.obj;
	}

	template<typename T>
	g_ptr<T> to_graph_ptr(gcpp::deferred_ptr<T> ptr) {
		return g_ptr<T>(ptr);
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
		using ptr_type = gcpp::deferred_ptr<U>;
	};

	template <>
	struct value_traits<field_value> {
		using obj_variant = std::variant<const_tile_graph_ptr, const_patch_graph_ptr, const_edge_graph_ptr, const_vertex_graph_ptr, const_lambda_graph_ptr, const_cluster_graph_ptr>;

		template<typename U>
		using ptr_type = g_ptr<U>;
	};

	template <typename V>
	bool is_simple_value(V v) {
		return std::holds_alternative<nil_val>(v) ||
			std::holds_alternative<number>(v) ||
			std::holds_alternative<std::string>(v) ||
			std::holds_alternative<bool>(v);
	};

	template <typename V>
	bool is_object_like(V v) {
		// by "object-like" we mean epression values that may have fields.
		return std::holds_alternative<const_tile_root_ptr>(v) ||
			std::holds_alternative<const_patch_root_ptr>(v) ||
			std::holds_alternative<const_vertex_root_ptr>(v) ||
			std::holds_alternative<const_edge_root_ptr>(v) ||
			std::holds_alternative<const_cluster_root_ptr>(v) ||
			std::holds_alternative<const_lambda_root_ptr>(v);
	}

	template <typename V>
	bool is_array_like(V v) {
		// by "array-like" we mean epression values that may be dereferenced via the [] operator.
		return  std::holds_alternative<const_patch_root_ptr>(v) ||
			std::holds_alternative<const_cluster_root_ptr>(v);
	}

	template <typename V>
	bool is_nil(V v) {
		return std::holds_alternative<tess::nil_val>(v);
	}

	value_ clone_value(gc_heap& allocator, value_ v);

	template<typename V>
	V clone_value(gc_heap& allocator, std::unordered_map<obj_id, std::any>& original_to_clone, V v) {
		using object_t = typename value_traits<V>::obj_variant;

		if (is_simple_value(v))
			return v;

		object_t obj_variant = variant_cast(v);
		return std::visit(
			[&](auto&& obj)->V { return make_value(detail::clone_aux(allocator, original_to_clone, obj)); },
			obj_variant
		);
	};

	value_ get_ary_item(value_ v, int index);
	int get_ary_count(value_ v);
	value_ get_field(value_ v, gc_heap& allocator, const std::string& field) ;
	void insert_field(value_ v, const std::string& var, value_ val);
	std::string to_string(value_ v);

	std::string serialize(value_ v);
	std::string serialize(serialization_state& state, value_ v);

	bool operator==(const value_& lhs, const value_& rhs);
	bool operator!=(const value_& lhs, const value_& rhs);

	template<typename T>
	value_ make_value(gcpp::deferred_ptr<T> v) {
		return { tess::to_const(v) };
	}

	template<typename T>
	field_value make_value(g_ptr<T> v) {
		return { tess::to_const(v) };
	}

	template<typename T, typename V>
	auto get_mutable(V val) {
		if constexpr ((std::is_same<T, field_ref_ptr>::value) || (std::is_same<T, nil_val>::value) || (std::is_same<T, number>::value) ||
			(std::is_same<T, std::string>::value) || (std::is_same<T, bool>::value)) {
			return std::get<T>(val);
		} else {
			using base_type = typename std::remove_const<typename T::value_type>::type;
			using ptr_type = typename value_traits<V>::ptr_type<base_type>;
			return ptr_type(std::get<T>(val));
		}
	}

	template<typename T>
	gcpp::deferred_ptr<T> clone(gc_heap& a, gcpp::deferred_ptr<const T> tess_impl) {
		tess::value_ val{ tess_impl };
		return gcpp::deferred_ptr<T>(std::get<gcpp::deferred_ptr<const T>>(tess::clone_value(a, val)));
	}

    template<typename T>
    gcpp::deferred_ptr<T> clone(gc_heap& a, gcpp::deferred_ptr< T> tess_impl) {
        return clone(a, gcpp::deferred_ptr<const T>(tess_impl));
    }
	
}