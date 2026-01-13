#include "dotlin/lexer.h"
#include "dotlin/parser.h"
#include <iostream>

int main() {
  std::string source = "println(\"test\")";
  std::cout << "Testing Dotlin lexer..." << std::endl;
  auto tokens = dotlin::tokenize(source);
  
  for (const auto &token : tokens)
  {
      std::cout << "Token: " << static_cast<int>(token.type)
                << ", Text: \"" << token.text
                << "\", Line: " << token.line
                << ", Col: " << token.column << std::endl;
  }
  
  std::cout << "\nTesting Dotlin parser..." << std::endl;
  auto program = dotlin::parse(tokens);
  std::cout << "Parsed " << program.statements.size() << " statements" << std::endl;
  
  return 0;
}
