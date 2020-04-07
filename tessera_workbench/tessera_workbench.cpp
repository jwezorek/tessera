#include "tessera/tessera.h"
#include "tessera/tessera_script.h"
#include "tessera/error.h"
#include <iostream>
#include <string>
#include <unordered_map>
#include <variant>
#include <fstream>
#include <sstream>
#include <algorithm>

std::string read_file(const std::string& file_path); 
std::vector<std::string> get_arguments(int argc, char** argv);
void generate_svg(const std::string& filename, const std::vector<tess::tile>& tiles, double scale);

int main(int argc, char** argv)
{
	std::string script = read_file("test.tess");
    auto results = tess::parse(script);

	if (std::holds_alternative<tess::tessera_script>(results)) {
		const auto& tessera = std::get<tess::tessera_script>(results);

		auto output = tessera.execute( get_arguments(argc, argv) );
		if (std::holds_alternative<tess::error>(output)) {
			std::cout << std::get<tess::error>(output) << "\n";
			return -1;
		}

		const auto& tiles = std::get<std::vector<tess::tile>>(output);
		generate_svg("C:\\test\\tiles.svg", tiles, 50.0 );

		std::cout << "success" << "\n";
	} else {
		auto err = std::get<tess::error>(results);
		std::cout << err << "\n";
	}
}

std::string read_file(const std::string& file_path) {
	std::ifstream file(file_path);
	return std::string(
		std::istreambuf_iterator<char>(file),
		std::istreambuf_iterator<char>()
	);
}

std::vector<std::string> get_arguments(int argc, char** argv) {
	std::vector<std::string> args;
	if (argc > 1) {
		args.resize(argc - 1);
		std::copy(argv + 1, argv + argc, args.begin() );
	}
	return args;
}

std::tuple<double, double, double, double> get_bounds(const std::vector<tess::tile>& tiles, float scale) {
	double x1 = std::numeric_limits<double>::max();
	double y1 = std::numeric_limits<double>::max();
	double x2 = std::numeric_limits<double>::min();
	double y2 = std::numeric_limits<double>::min();

	for (const auto& tile : tiles) {
		for (const auto& vertex : tile.vertices()) {
			auto [x, y] = vertex.pos();
			x *= scale;
			y *= scale;
			if (x < x1)
				x1 = x;
			if (x > x2)
				x2 = x;
			if (y < y1)
				y1 = y;
			if (y > y2)
				y2 = y;
		}
	}

	float wd = x2 - x1;
	float hgt = y2 - y1;

	return { x1 - scale, y1 - scale, wd + 2*scale, hgt + 2*scale };
}

std::string tile_to_svg(const tess::tile& tile, double scale) {
	std::stringstream ss;

	ss << "<polygon stroke=\"black\" fill=\"yellow\" points = \"";
	for (const auto& vertex : tile.vertices()) {
		auto [x, y] = vertex.pos();
		ss << x*scale << "," << y*scale << " ";
	}
	ss << "\" />";

	return ss.str();
}

void generate_svg(const std::string& filename, const std::vector<tess::tile>& tiles, double scale) {
	std::ofstream out(filename);

	auto [x, y, wd, hgt] = get_bounds(tiles, scale);
	out << "<svg viewBox = \"" << x << " " << y << " " << wd << " " << hgt << "\" xmlns = \"http://www.w3.org/2000/svg\">\n";
	for (const auto& tile : tiles)
		out << tile_to_svg(tile, scale) << "\n";
	out << "</svg>\n";

	out.close();
}
