#include <iostream>
#include <string>
#include <unordered_map>
#include <variant>
#include "tessera/tessera.h"
#include "tessera/tesserascript.h"
#include "tessera/error.h"

int main()
{
	std::string script = "tile foo { stuff }";
    auto results = tess::parse(script);

	if (std::holds_alternative<tess::tessera_script>(results)) {
		std::cout << "success" << "\n";
	} else {
		auto err = std::get<tess::error>(results);
		std::cout << err << "\n";
	}
}

