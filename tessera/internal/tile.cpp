#include "..\include\tessera\tile.h"

std::string tess::tile::name() const
{
	return std::string();
}

const std::vector<tess::vertex>& tess::tile::vertices() const
{
	return {};
}

const std::vector<tess::edge>& tess::tile::edges() const
{
	return {};
}

const tess::vertex& tess::tile::vertex(const std::string& vert) const
{
	return {};
}

const tess::edge& tess::tile::edge(const std::string& e) const
{
	return {};
}

std::string tess::vertex::name() const
{
	return std::string();
}

std::string tess::vertex::class_() const
{
	return std::string();
}

std::string tess::edge::name() const
{
	return std::string();
}

std::string tess::edge::class_() const
{
	return std::string();
}
