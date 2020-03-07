#pragma once

#include "tile_patch.h"
#include <string>
#include <vector>
#include <memory>

namespace tess {

	class tableau 
	{
	private:
		class tableau_impl;
		std::shared_ptr<tableau_impl> impl_;
	public:
		std::string name() const;
		const std::vector<tile_patch>& patches() const;
	};

}