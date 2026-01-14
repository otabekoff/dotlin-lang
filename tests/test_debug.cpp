#include "dotlin/interpreter.h"
#include "dotlin/lexer.h"
#include "dotlin/parser.h"
#include <iostream>
#include <fstream>
#include <sstream>

std::string readFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filepath);
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <file.lin>" << std::endl;
        return 1;
    }
    
    std::string filepath = argv[1];
    std::string source = readFile(filepath);
    
    std::cout << "Source code:" << std::endl;
    std::cout << source << std::endl << std::endl;
    
    auto tokens = dotlin::tokenize(source);
    std::cout << "Tokens:" << std::endl;
    for (size_t i = 0; i < tokens.size(); ++i) {
        std::cout << "  " << i << ": " << static_cast<int>(tokens[i].type) 
                  << " \"" << tokens[i].text << "\" at " << tokens[i].line 
                  << ":" << tokens[i].column << std::endl;
    }
    std::cout << std::endl;
    
    auto program = dotlin::parse(tokens);
    std::cout << "Parsed " << program.statements.size() << " statements" << std::endl;
    
    auto result = dotlin::interpret(program);
    std::cout << "Execution completed." << std::endl;
    
    return 0;
}
