#pragma once

#include "value.h"
#include <memory>

namespace tess {
	class field_ref_impl
	{
	public:
		class impl_type;
		field_ref_impl() {}
		field_ref_impl( value_ obj, std::string field);
		void set( value_ val);

	protected:
		value_ obj_;
		std::string field_;
	};

}