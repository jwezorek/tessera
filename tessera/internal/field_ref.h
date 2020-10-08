#pragma once

#include "value.h"
#include <memory>

namespace tess {
	class field_ref_impl
	{
	public:
		class impl_type;
		field_ref_impl() {}
		field_ref_impl( expr_value obj, std::string field);
		void set( expr_value val);

	protected:
		expr_value obj_;
		std::string field_;
	};

}