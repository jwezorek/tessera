#pragma once

#include <string>
#include <vector>
#include <memory>
#include <tuple>

namespace tess {

	class vertex {
		friend class tile_def;
		friend class tile;
	private:
		class vertex_impl;
		std::shared_ptr<vertex_impl> impl_;

		vertex(const std::shared_ptr<vertex_impl>& impl);

	public:
		vertex(); //TODO: get rid of the need for this
		std::string name() const;
		std::string vertex_class() const;
		std::tuple<double, double> pos() const;
	};

	class edge {
	private:
		class edge_impl;
		std::shared_ptr<edge_impl> impl_;
	public:
		std::string name() const;
		std::string edge_class() const;
		const vertex& u() const;
		const vertex& v() const;
	};

	class tile
	{
		friend class tile_def;
		friend class edge;
		friend class vertex;
	private:
		class tile_impl;
		std::shared_ptr<tile_impl> impl_;

		tile(std::shared_ptr<tile_impl> impl);

	public:
		std::string name() const;
		const std::vector<vertex>& vertices() const;
		const std::vector<edge>& edges() const;
	};

}