#pragma once
#include "../include/tessera/tile.h"
#include "../include/tessera/tile_patch.h"
#include "../include/tessera/error.h"
#include "cluster.h"
#include "tessera_impl.h"
#include "lambda.h"
#include "number.h"
#include <variant>
#include <memory>
#include <unordered_set>
#include <unordered_map>

namespace tess {

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

	using expr_val_var = std::variant<tile::impl_type*, tile_patch::impl_type*, edge::impl_type*, vertex::impl_type*, lambda::impl_type*, cluster::impl_type*, field_ref, nil_val, number, std::string, bool>;

	class expr_value : public expr_val_var, public tessera_impl
	{
	public:
		expr_value();
		explicit expr_value(tile::impl_type* v);
		explicit expr_value(const tile::impl_type* v);
		explicit expr_value(tile_patch::impl_type* v);
		explicit expr_value(const tile_patch::impl_type* v);
		explicit expr_value(edge::impl_type* v);
		explicit expr_value(const edge::impl_type* v);
		explicit expr_value(vertex::impl_type*);
		explicit expr_value(const vertex::impl_type*);
		explicit expr_value(lambda::impl_type*);
		explicit expr_value(const lambda::impl_type*);
		explicit expr_value(cluster::impl_type*);
		explicit expr_value(const cluster::impl_type*);
		explicit expr_value(field_ref);
		explicit expr_value(nil_val);
		explicit expr_value(number);
		explicit expr_value(std::string);
		explicit expr_value(bool);

		bool is_simple_value() const;
		bool is_object_like() const;
		bool is_array_like() const;
		bool is_nil() const;
		expr_value clone( allocator& allocator ) const;
		expr_value clone( allocator& allocator, std::unordered_map<obj_id,void*>& original_to_clone) const;
		expr_value get_ary_item(int index) const;
		int get_ary_count() const;
		expr_value get_field(allocator& allocator, const std::string& field) const;
		void insert_field(const std::string& var, expr_value val) const;
		std::unordered_set<obj_id> get_all_referenced_allocations() const;
		void get_all_referenced_allocations(std::unordered_set<obj_id>& alloc_set) const;
		std::string to_string() const;
	};

	bool operator==(const expr_value& lhs, const expr_value& rhs);
	bool operator!=(const expr_value& lhs, const expr_value& rhs);

	template<typename T> T* from_void_star(void* ptr) {
		return reinterpret_cast<T*>(ptr);
	}

	template<typename T>
	T* clone(allocator& a, const T* tess_impl) {
		tess::expr_value val{ const_cast<T*>(tess_impl) };
		return std::get<T*>(val.clone(a));
	}

	template<typename T>
	T* clone(allocator& a, T* tess_impl) {
		tess::expr_value val{ tess_impl };
		return std::get<T*>(val.clone(a));
	}
	
}