#include <iostream>
#include "tessera/tessera.h"

int main()
{
	std::string script = "foo:22 - foo^2 \n * (1.0 / bar)";
    auto results = tess::parse(script);
    std::cout << "hello\n";
}

