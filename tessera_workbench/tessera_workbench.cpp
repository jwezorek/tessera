#include <iostream>
#include "tessera/tessera.h"

int main()
{
	std::string script = "22 - foo^2 * (1.0 / bar)";
    auto results = tess::parse(script);
    std::cout << "hello\n";
}

