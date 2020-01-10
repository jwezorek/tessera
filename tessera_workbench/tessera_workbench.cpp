#include <iostream>
#include "tessera/tesserascript.h"

int main()
{
    auto results = tess::parse("aaa bbb hello\n monkey  foo aaa bar\n goodbye\n");
    std::cout << "hello\n";
}

