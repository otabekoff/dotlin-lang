#include "dotlin/lexer.h"
#include "dotlin/parser.h"
#include "dotlin/interpreter.h"
#include <iostream>

int main() {
    // Simple test
    std::string source = "val x = 42";
    
    std::cout << "Testing Dotlin lexer..." << std::endl;
    auto tokens = dotlin::tokenize(source);
    
    for (const auto& token : tokens) {
        std::cout << "Token: " << static_cast<int>(token.type) 
                  << ", Text: " << token.text 
                  << ", Line: " << token.line 
                  << ", Col: " << token.column << std::endl;
    }
    
    std::cout << "\nTesting Dotlin parser..." << std::endl;
    auto program = dotlin::parse(tokens);
    std::cout << "Parsed " << program.statements.size() << " statements" << std::endl;
    
    std::cout << "\nTesting Dotlin interpreter..." << std::endl;
    auto result = dotlin::interpret(program);
    
    if (std::holds_alternative<std::string>(result)) {
        std::cout << "Interpreter result: " << std::get<std::string>(result) << std::endl;
    } else if (std::holds_alternative<int>(result)) {
        std::cout << "Interpreter result: " << std::get<int>(result) << std::endl;
    } else if (std::holds_alternative<double>(result)) {
        std::cout << "Interpreter result: " << std::get<double>(result) << std::endl;
    } else if (std::holds_alternative<bool>(result)) {
        std::cout << "Interpreter result: " << (std::get<bool>(result) ? "true" : "false") << std::endl;
    }
    
    return 0;
}