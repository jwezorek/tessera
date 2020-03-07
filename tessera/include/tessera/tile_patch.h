#pragma once

#include <string>
#include <memory>

namespace tess {

	class tile_patch
	{
	private:
		class tile_patch_impl;
		std::shared_ptr<tile_patch_impl> impl_;
	public:
		std::string name() const;
	};

}