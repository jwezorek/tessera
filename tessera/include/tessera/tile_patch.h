#pragma once

#include "tile.h"
#include <string>
#include <vector>
#include <memory>

namespace tess {

	class tile_patch_impl;

	class tile_patch
	{
		friend tessera_impl;
	private:
		std::shared_ptr<tile_patch_impl> impl_;
	public:
		std::string name() const;
		const std::vector<tile>& tiles() const;
		int count() const;
	};

}