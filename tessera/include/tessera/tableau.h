#pragma once

#include <string>
#include <memory>

namespace tess {

	class tableau 
	{
	private:
		class tableau_impl;
		std::shared_ptr<tableau_impl> impl_;
	public:
		std::string name() const;
	};

}