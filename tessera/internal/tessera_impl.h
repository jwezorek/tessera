#pragma once

#include <memory>

namespace tess {

	using obj_id = uint64_t;

    class tessera_impl {
		private:
			obj_id id_;
        public:

			tessera_impl(obj_id id = 0) : id_(id) {}

			obj_id get_id() const { return id_; }

			template<typename T>
			const typename T::impl_type* get_impl(const T& tess_obj) const {
				return tess_obj.impl_;
			}

			template<typename T>
			typename T::impl_type* get_impl(T& tess_obj) const {
				return tess_obj.impl_;
			}

			template<typename T>
			T make_tess_obj(typename T::impl_type* impl) const {
				T obj;
				obj.impl_ = impl;
				return obj;
			}

			template<typename T>
			T make_tess_obj(const typename T::impl_type* impl) const {
				T obj;
				obj.impl_ = const_cast<typename T::impl_type*>(impl);
				return obj;
			}

			template<typename T>
			T make_tess_obj(typename std::shared_ptr<typename T::impl_type> impl) const {
				T obj;
				obj.impl_ = impl;
				return obj;
			}

    };

	namespace detail {
		struct impl_getter : tessera_impl {
			template<typename U>
			typename U::impl_type* operator()(U& t) {
				return get_impl(t);
			}

			template<typename U>
			const typename U::impl_type* operator()(const U& t) {
				return get_impl(t);
			}
		};

		struct tess_obj_maker : tessera_impl {

			template<typename U>
			U make(typename U::impl_type* impl) {
				return make_tess_obj<U>(impl);
			}

			template<typename U>
			U make(const typename U::impl_type* impl) {
				return make_tess_obj<U>(impl);
			}
		};

	}

	template<typename T>
	typename T::impl_type* get_impl(T& tess_obj)
	{
		detail::impl_getter getter;
		return getter(tess_obj);
	}

	template<typename T>
	const typename T::impl_type* get_impl(const T& tess_obj)
	{
		detail::impl_getter getter;
		return getter(tess_obj);
	}

	template<typename T>
	T make_tess_obj(typename T::impl_type* impl) {
		detail::tess_obj_maker maker;
		return maker.make<T>(impl);
	}

	template<typename T>
	T make_tess_obj(const typename T::impl_type* impl) {
		detail::tess_obj_maker maker;
		return maker.make<T>(impl);
	}
}