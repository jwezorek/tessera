#pragma once

#include <string>
#include <vector>
#include <memory>
#include <tuple>

namespace tess {

	class vertex {
	private:
		class vertex_impl_;
		std::shared_ptr<vertex_impl_> impl_;
	public:
		std::string name() const;
		std::string class_() const;
		std::tuple<double, double> pos;
	};

	class edge {
	private:
		class edge_impl_;
		std::shared_ptr<edge_impl_> impl_;
	public:
		std::string name() const;
		std::string class_() const;
		const vertex& u() const;
		const vertex& v() const;
	};

	class tile
	{
	private:
		class tile_impl;
		std::shared_ptr<tile_impl> impl_;
	public:
		std::string name() const;
		const std::vector<vertex>& vertices() const;
		const std::vector<edge>& edges() const;
		const vertex& vertex(const std::string& vert) const;
		const edge& edge(const std::string& e) const;
	};

}