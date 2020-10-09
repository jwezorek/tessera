#pragma once

#include "tile.h"
#include <string>
#include <vector>
#include <memory>

namespace tess {

	namespace detail {
		class patch_impl;
	}

	class tile_patch : public detail::property_container<tile_patch>
	{
		friend class tessera_impl;
	public:
		std::vector<tile> tiles() const;
		int count() const;
		property_value get_property_variant(const std::string& prop) const;
		using impl_type = detail::patch_impl;
	private:
		const impl_type* impl_;
	};
}