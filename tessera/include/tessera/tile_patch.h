#pragma once

#include "tile.h"
#include <string>
#include <vector>
#include <memory>

namespace tess {

	class tile_patch
	{
		friend class tessera_impl;
	public:
		class impl_type;
	private:
		std::shared_ptr<impl_type> impl_;
	public:
		std::string name() const;
		const std::vector<tile>& tiles() const;
		int count() const;
	};

}