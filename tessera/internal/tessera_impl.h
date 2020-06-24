#pragma once

#include <memory>

namespace tess {

    class tessera_impl {
        public:

			template<typename T>
			typename T::impl_type* get_impl(T& tess_obj) const {
				return tess_obj.impl_;
			}

			template<typename T, typename I>
			T make_tess_obj(I impl) const {
				T obj;
				obj.impl_ = impl;
				return obj;
			}
    };

	namespace details {
		struct impl_getter : tessera_impl {
			template<typename U>
			typename U::impl_type* operator()(U& t) {
				return get_impl(t);
			}
		};
	}

	template<typename T>
	typename T::impl_type* get_impl(T& tess_obj)
	{
		details::impl_getter getter;
		return getter(tess_obj);
	}

	template<typename T>
	const typename T::impl_type* get_impl(const T& tess_obj)
	{
		details::impl_getter getter;
		return getter(tess_obj);
	}

}