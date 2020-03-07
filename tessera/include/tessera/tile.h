#pragma once

#include <string>
#include <memory>

namespace tess {

	class tile
	{
	private:
		class tile_impl;
		std::shared_ptr<tile_impl> impl_;
	public:
		std::string name() const;
	};

}