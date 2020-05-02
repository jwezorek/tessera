#pragma once

#include <memory>

namespace tess {

    class tessera_impl {
        public:
			template<typename T, typename... Args>
			T make_tess_obj(Args... args) const {
				T obj;
				obj.impl_ = std::make_shared<typename T::impl_type>(std::forward<Args>(args)...);
				return obj;
			}

			template<typename T, typename I>
			T make_tess_obj_from_impl(const std::shared_ptr<I>& impl) const {
				T obj;
				obj.impl_ = impl;
				return obj;
			}

			template<typename T>
			std::shared_ptr<typename T::impl_type> get_impl(T& tess_obj) const {
				return tess_obj.impl_;
			}
			///
			template<typename T>
			typename T::impl_type* get_implementation(T& tess_obj) const {
				return tess_obj.impl_;
			}

			template<typename T, typename I>
			T make_tess_obj_from_impl2(I* impl) const {
				T obj;
				obj.impl_ = impl;
				return obj;
			}
    };

}