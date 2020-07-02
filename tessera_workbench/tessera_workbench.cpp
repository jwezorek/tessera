#include "tessera/script.h"
#include "tessera/error.h"
#include <iostream>
#include <string>
#include <unordered_map>
#include <variant>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <chrono>
#include <numeric>

std::string read_file(const std::string& file_path); 
std::tuple<std::string, std::string, std::vector<std::string>> get_arguments(int argc, char** argv);
void generate_svg(const std::string& filename, const std::vector<tess::tile>& tiles, double scale);
std::string generate_output_filename(const std::string& filename, const std::string& out_dir);
std::string args_to_string(std::vector<std::string> args);
namespace fs = std::filesystem;

int main(int argc, char** argv){
	auto [script_file_path, output_directory, tessera_args] = get_arguments(argc, argv);
	auto script_name = fs::path(script_file_path).filename().string();
	std::string source_code = read_file(script_file_path);

	std::cout << "parsing " << script_name << " ...\n";
	auto results = tess::script::parse( source_code );
	if (std::holds_alternative<tess::script>(results)) {
		std::cout << "  success.\n\n";
	} else {
		auto err = std::get<tess::error>(results);
		std::cout << err << "\n";
		return -1;
	}

	const auto& tessera = std::get<tess::script>(results);
		
	std::cout << "executing " << script_name << " on " << args_to_string(tessera_args) << "...\n";
	auto output = tessera.execute(tessera_args);
	if (!std::holds_alternative<tess::error>(output)) {
		std::cout << "  success.\n\n"; 
	} else {
		std::cout << std::get<tess::error>(output) << "\n";
		return -1;
	}

	std::cout << "processing " << script_name << " tiles ...\n";
	const auto& tiles = std::get<std::vector<tess::tile>>(output);
	auto output_file = generate_output_filename(script_file_path, output_directory);
	generate_svg(output_file, tiles, 30.0);

	std::cout << "  complete." << "\n";
	std::cout << "  generated " << output_file << "\n";
}

std::string read_file(const std::string& file_path) {
	std::ifstream file(file_path);
	return std::string(
		std::istreambuf_iterator<char>(file),
		std::istreambuf_iterator<char>()
	);
}

std::tuple<std::string, std::string, std::vector<std::string>> get_arguments(int argc, char** argv) {
	std::vector<std::string> args;
	if (argc < 2)
		return {};
	if (argc > 1) {
		args.resize(argc - 1);
		std::copy(argv + 1, argv + argc, args.begin() );
	}
	return { args[0], args[1], std::vector<std::string>(args.begin()+2,args.end()) };
}

std::tuple<double, double, double, double> get_bounds(const std::vector<tess::tile>& tiles, float scale) {
	double x1 = std::numeric_limits<double>::max();
	double y1 = std::numeric_limits<double>::max();
	double x2 = std::numeric_limits<double>::min();
	double y2 = std::numeric_limits<double>::min();

	int i = 1;
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

	auto wd = x2 - x1;
	auto hgt = y2 - y1;

	return { x1 - scale, y1 - scale, wd + 2*scale, hgt + 2*scale };
}

std::string tile_to_svg(const tess::tile& tile, double scale) {
	std::stringstream ss;
	auto maybe_color = tile.get_prop<std::string>("color");
	std::string color = (maybe_color.has_value()) ? maybe_color.value() : "white";
	ss << "<polygon stroke=\"black\" fill=\"" << color << "\" points = \"";
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

std::string generate_output_filename(const std::string& script_file, const std::string& dir) {
	fs::path input = script_file;
	fs::path output = fs::path(dir) / input.filename();
	output.replace_extension(".svg");
	return output.string();
}

std::string args_to_string(std::vector<std::string> args) {
	auto comma_delimited_args = std::accumulate(
		std::next(args.begin()),
		args.end(),
		args[0],
		[](std::string a, std::string b) {
			return a + std::string(" , ") + b;
		}
	);
	return "( " + comma_delimited_args + " )";
}