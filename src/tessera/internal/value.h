#pragma once

#include "tessera_impl.h"
#include "number.h"
#include "variant_util.h"
#include <variant>
#include <memory>
#include <unordered_set>
#include <unordered_map>

namespace tess {

	namespace detail {
		class vertex_impl;
		class edge_impl;
		class cluster_impl;
		class tile_impl;
		class patch_impl;
		class lambda_impl;
	}

	class allocator;
	class execution_state;

    class nil_val {
    public:
        nil_val();
    };

	bool operator==(nil_val lhs, nil_val rhs);

	class field_ref_impl;
	using field_ref_ptr = std::shared_ptr<field_ref_impl>;

	using const_vertex_ptr = const detail::vertex_impl*;
	using const_edge_ptr = const detail::edge_impl*;
	using const_cluster_ptr = const detail::cluster_impl*;
	using const_tile_ptr = const detail::tile_impl*;
	using const_patch_ptr = const detail::patch_impl*;
	using const_lambda_ptr = const detail::lambda_impl*;
	using vertex_ptr =  detail::vertex_impl*;
	using edge_ptr =  detail::edge_impl*;
	using cluster_ptr =  detail::cluster_impl*;
	using tile_ptr =  detail::tile_impl*;
	using patch_ptr =  detail::patch_impl*;
	using lambda_ptr =  detail::lambda_impl*;

	using value_ = std::variant<const_tile_ptr, const_patch_ptr, const_edge_ptr, const_vertex_ptr, const_lambda_ptr, const_cluster_ptr, field_ref_ptr, nil_val, number, std::string, bool>;

	bool is_simple_value(value_);
	bool is_object_like(value_);
	bool is_array_like(value_);
	bool is_nil(value_);
	value_ clone_value(allocator& allocator, value_ v);
	value_ clone_value(allocator& allocator, std::unordered_map<obj_id, void*>& original_to_clone, value_ v);
	value_ get_ary_item(value_ v, int index);
	int get_ary_count(value_ v);
	value_ get_field(value_ v, allocator& allocator, const std::string& field) ;
	void insert_field(value_ v, const std::string& var, value_ val);
	std::unordered_set<obj_id> get_references(value_ v);
	void get_references(value_ v, std::unordered_set<obj_id>& alloc_set);
	std::string to_string(value_ v);

	bool operator==(const value_& lhs, const value_& rhs);
	bool operator!=(const value_& lhs, const value_& rhs);

	template<typename T>
	auto get_mutable(value_ val) -> auto {
		if constexpr ((std::is_same<T, field_ref_ptr>::value) || (std::is_same<T, nil_val>::value) || (std::is_same<T, number>::value) ||
			(std::is_same<T, std::string>::value) || (std::is_same<T, bool>::value)) {
			return std::get<T>(val);
		} else {
			using base_type = typename std::remove_const<typename std::remove_pointer<T>::type>::type;
			return const_cast<base_type*>(std::get<T>(val));
		}
	}

	template <typename T>
	void* to_void_star(const T* obj_ptr) {
		T* non_const_ptr = const_cast<T*>(obj_ptr);
		return reinterpret_cast<void*>(non_const_ptr);
	}

	template<typename T> T* from_void_star(void* ptr) {
		return reinterpret_cast<T*>(ptr);
	}

	template<typename T>
	T* clone(allocator& a, const T* tess_impl) {
		tess::value_ val{ tess_impl };
		return get_mutable<const T*>(tess::clone_value(a, val));
	}
	
}