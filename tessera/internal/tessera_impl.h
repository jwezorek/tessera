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

}