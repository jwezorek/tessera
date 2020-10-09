#pragma once

#include "tessera_impl.h"
#include "number.h"
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

	using value_variant = std::variant<const_tile_ptr, const_patch_ptr, const_edge_ptr, const_vertex_ptr, const_lambda_ptr, const_cluster_ptr, field_ref_ptr, nil_val, number, std::string, bool>;

	class value_ : public value_variant
	{
	public:
		value_();
		explicit value_(const_tile_ptr v);
		explicit value_(const_patch_ptr v);
		explicit value_(const_edge_ptr v);
		explicit value_(const_vertex_ptr);
		explicit value_(const_lambda_ptr);
		explicit value_(const_cluster_ptr);
		explicit value_(field_ref_ptr);
		explicit value_(nil_val);
		explicit value_(number);
		explicit value_(std::string);
		explicit value_(bool);
	};

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
	std::unordered_set<obj_id> get_all_referenced_allocations(value_ v);
	void get_all_referenced_allocations(value_ v, std::unordered_set<obj_id>& alloc_set);
	std::string to_string(value_ v);

	bool operator==(const value_& lhs, const value_& rhs);
	bool operator!=(const value_& lhs, const value_& rhs);

	template<typename T> T* from_void_star(void* ptr) {
		return reinterpret_cast<T*>(ptr);
	}

	template<typename T>
	T* clone(allocator& a, const T* tess_impl) {
		tess::value_ val{ const_cast<T*>(tess_impl) };
		auto const_ptr = std::get<const T*>(tess::clone_value(a, val));
		return const_cast<T*>(const_ptr);
	}

	template<typename T>
	T* clone(allocator& a, T* tess_impl) {
		tess::value_ val{ tess_impl };
		return std::get<T*>(tess::clone_value(a, val));
	}
	
}