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

	class expr_value;
	class field_ref
	{
		friend class tessera_impl;
		friend bool operator==(field_ref lhs, field_ref rhs);
	public:
		class impl_type;
		field_ref() {}
		field_ref(const expr_value& obj, std::string field);
		void set(const expr_value& val);

	protected:
		std::shared_ptr<impl_type> impl_;
	};

	bool operator==(field_ref lhs, field_ref rhs);

	using vertex_ptr = detail::vertex_impl*;
	using const_vertex_ptr = const detail::vertex_impl*;
	using edge_ptr = detail::edge_impl*;
	using const_edge_ptr = const detail::edge_impl*;
	using cluster_ptr = detail::cluster_impl*;
	using const_cluster_ptr = const detail::cluster_impl*;;
	using tile_ptr = detail::tile_impl*;
	using const_tile_ptr = const detail::tile_impl*;
	using patch_ptr = detail::patch_impl*;
	using const_patch_ptr = const detail::patch_impl*;
	using lambda_ptr = detail::lambda_impl*;
	using const_lambda_ptr = const detail::lambda_impl*;

	using expr_val_var = std::variant<tile_ptr, patch_ptr, edge_ptr, vertex_ptr, lambda_ptr, cluster_ptr, field_ref, nil_val, number, std::string, bool>;

	class expr_value : public expr_val_var
	{
	public:
		expr_value();
		explicit expr_value(tile_ptr v);
		explicit expr_value(const_tile_ptr v);
		explicit expr_value(patch_ptr v);
		explicit expr_value(const_patch_ptr v);
		explicit expr_value(edge_ptr v);
		explicit expr_value(const_edge_ptr v);
		explicit expr_value(vertex_ptr);
		explicit expr_value(const_vertex_ptr);
		explicit expr_value(lambda_ptr);
		explicit expr_value(const_lambda_ptr);
		explicit expr_value(cluster_ptr);
		explicit expr_value(const_cluster_ptr);
		explicit expr_value(field_ref);
		explicit expr_value(nil_val);
		explicit expr_value(number);
		explicit expr_value(std::string);
		explicit expr_value(bool);
	};

	bool is_simple_value(expr_value);
	bool is_object_like(expr_value);
	bool is_array_like(expr_value);
	bool is_nil(expr_value);
	expr_value clone_value(allocator& allocator, expr_value v);
	expr_value clone_value(allocator& allocator, std::unordered_map<obj_id, void*>& original_to_clone, expr_value v);
	expr_value get_ary_item(expr_value v, int index);
	int get_ary_count(expr_value v);
	expr_value get_field(expr_value v, allocator& allocator, const std::string& field) ;
	void insert_field(expr_value v, const std::string& var, expr_value val);
	std::unordered_set<obj_id> get_all_referenced_allocations(expr_value v);
	void get_all_referenced_allocations(expr_value v, std::unordered_set<obj_id>& alloc_set);
	std::string to_string(expr_value v);

	bool operator==(const expr_value& lhs, const expr_value& rhs);
	bool operator!=(const expr_value& lhs, const expr_value& rhs);

	template<typename T> T* from_void_star(void* ptr) {
		return reinterpret_cast<T*>(ptr);
	}

	template<typename T>
	T* clone(allocator& a, const T* tess_impl) {
		tess::expr_value val{ const_cast<T*>(tess_impl) };
		return std::get<T*>(tess::clone_value(a, val));
	}

	template<typename T>
	T* clone(allocator& a, T* tess_impl) {
		tess::expr_value val{ tess_impl };
		return std::get<T*>(tess::clone_value(a, val));
	}
	
}