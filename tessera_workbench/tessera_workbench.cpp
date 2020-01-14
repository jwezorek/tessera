#include <iostream>
#include "tessera/tesserascript.h"

int main()
{
	std::string script = "if edge _ lay tableau;\n tile = -1.5 else/foobar_12 else else\n";
    auto results = tess::parse(script);
    std::cout << "hello\n";
}

