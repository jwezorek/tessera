#include <iostream>
#include "tessera/tesserascript.h"

int main()
{
	std::string script = "if edge lay tableau\n tile else  else else\n";
    auto results = tess::parse(script);
    std::cout << "hello\n";
}

