#pragma once

#include "tile.h"
#include <string>
#include <vector>
#include <memory>

namespace tess {

	class tile_patch
	{
		friend class allocator;

	public:
		std::string name() const;
		const std::vector<tile>& tiles() const;
		int count() const;

		class impl_type;
		
	private:
		impl_type* impl_;
	};

}