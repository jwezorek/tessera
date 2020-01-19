#include <iostream>
#include <string>
#include <unordered_map>
#include <variant>
#include "tessera/tessera.h"
#include "tessera/tesserascript.h"
#include "tessera/error.h"

int main()
{
	std::string script = "foo:(1.0 / monkey + monkey/ bar)";
    auto results = tess::parse(script);

	if (std::holds_alternative<tess::tessera_script>(results)) {
		auto script = std::get<tess::tessera_script>(results);
		std::unordered_map<std::string, double> vars = {
		{"foo", 2},
		{"bar", 1},
		{"monkey", 4}
		};
		std::cout << script.evaluate(vars) << "\n";
	}
	else {
		auto err = std::get<tess::error>(results);
		std::cout << err << "\n";
	}
}

