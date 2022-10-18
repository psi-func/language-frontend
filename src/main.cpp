#include <iostream>

#include "parser.h"

int main()
{
    // Install standard binary operators
    // 1 is lowest precendence.
    BinopPrecendence['<'] = 10;
    BinopPrecendence['+'] = 20;
    BinopPrecendence['-'] = 30;
    BinopPrecendence['*'] = 40; // highest

    MainLoop();

    return 0;
}