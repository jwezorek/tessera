#pragma once

#include "tile.h"
#include <string>
#include <vector>
#include <memory>

namespace tess {

	class tile_patch : public detail::property_container<tile_patch>
	{
		friend class tessera_impl;
	public:
		std::string name() const;
		const std::vector<tile>& tiles() const;
		int count() const;
		property_value get_property_variant(const std::string& prop) const;
		class impl_type;
	private:
		impl_type* impl_;
	};

	bool operator==(tile_patch l, tile_patch r);
}