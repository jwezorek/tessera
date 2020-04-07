#include "tessera/tessera.h"
#include "tessera/tessera_script.h"
#include "tessera/error.h"
#include <iostream>
#include <string>
#include <unordered_map>
#include <variant>
#include <fstream>

std::string read_file(const std::string& file_path); 

int main()
{
	std::string script = read_file("test.tess");
    auto results = tess::parse(script);

	if (std::holds_alternative<tess::tessera_script>(results)) {
		const auto& tessera = std::get<tess::tessera_script>(results);

		auto output = tessera.execute( "5" );
		if (std::holds_alternative<tess::error>(output)) {
			std::cout << std::get<tess::error>(output) << "\n";
			return -1;
		}

		const auto& tiles = std::get<std::vector<tess::tile>>(output);
		for (const auto& tri : tiles) {
			for (const auto& v : tri.vertices()) {
				auto [x, y] = v.pos();
				std::cout << "( " << x << " , " << y << " ) ";
			}
			std::cout << "\n";
		}

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
