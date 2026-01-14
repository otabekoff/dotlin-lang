#include "dotlin/lexer.h"
#include <iostream>

int main()
{
    std::string source = "val x = 42";

    std::cout << "Testing Dotlin lexer..." << std::endl;
    auto tokens = dotlin::tokenize(source);

    for (const auto &token : tokens)
    {
        std::cout << "Token: " << static_cast<int>(token.type)
                  << ", Text: " << token.text
                  << ", Line: " << token.line
                  << ", Col: " << token.column << std::endl;
    }

    return 0;
}