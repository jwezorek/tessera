#pragma once

#include <string>
#include <vector>
#include <memory>
#include <tuple>

namespace tess {
	class edge_impl;
	class vertex_impl;
	class tile_impl;

	class vertex {
		friend class tile_def;
	private:
		std::shared_ptr<vertex_impl> impl_;
		vertex(const std::shared_ptr<vertex_impl>& impl);
		
	public:
		vertex() {}
		std::string name() const;
		std::string vertex_class() const;
		std::tuple<double, double> pos() const;
	};

	class edge {
		friend class tile_def;
	private:
		std::shared_ptr<edge_impl> impl_;
		edge(const std::shared_ptr<edge_impl>& impl);
	public:
		edge(){}
		std::string name() const;
		std::string edge_class() const;
		const vertex& u() const;
		const vertex& v() const;
	};

	class tile
	{
		friend class tile_def;
	private:
		std::shared_ptr<tile_impl> impl_;
		tile(std::shared_ptr<tile_impl> impl);

	public:
		std::string name() const;
		const std::vector<vertex>& vertices() const;
		const std::vector<edge>& edges() const;
	};

}