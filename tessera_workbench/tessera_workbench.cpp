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
