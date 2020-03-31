#pragma once

#include <memory>

namespace tess {

    class tessera_impl {
        public:
			template<typename T, typename I>
			T make_tess_obj(const std::shared_ptr<I>& impl) const {
				T obj;
				obj.impl_ = impl;
				return obj;
			}

			template<typename T, typename I>
			std::shared_ptr<I> get_impl(T& tess_obj) {
				return tess_obj.impl_;
			}

    };

}