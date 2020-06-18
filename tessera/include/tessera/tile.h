#pragma once

#include <string>
#include <vector>
#include <memory>
#include <tuple>

namespace tess {
	class tile_impl;

	class vertex {
		friend class tessera_impl;
	public:
		std::tuple<double, double> pos() const;
		class impl_type;
	private:
		impl_type* impl_;
	};

	class edge {
		friend class tessera_impl;
	public:
		const vertex& u() const;
		const vertex& v() const;
		class impl_type;
	private:
		impl_type* impl_;
	};

	class tile
	{
		friend class tessera_impl;
	public:
		const std::vector<vertex>& vertices() const;
		const std::vector<edge>& edges() const;
		class impl_type;
	private:
		impl_type* impl_;
	};

}