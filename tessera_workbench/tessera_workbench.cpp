#include <iostream>
#include <string>
#include <unordered_map>
#include "tessera/tessera.h"
#include "tessera/tesserascript.h"

int main()
{
	std::string script = "foo:22 - foo^2 \n * (1.0 / bar)";
    auto results = tess::parse(script);

    auto script1 = std::get<tess::tessera_script>(results);

    std::unordered_map<std::string, double> vars = {
        {"foo", 2},
        {"bar", 1}
    };

    std::cout << script1.evaluate(vars) << "\n";
}

