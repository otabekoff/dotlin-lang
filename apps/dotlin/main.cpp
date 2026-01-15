#include "dotlin/interpreter.h"
#include "dotlin/lexer.h"
#include "dotlin/parser.h"
// #include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

std::string readFile(const std::string &filepath) {
  std::ifstream file(filepath);
  if (!file.is_open()) {
    throw std::runtime_error("Could not open file: " + filepath);
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

bool hasExtension(const std::string &filename, const std::string &ext) {
  if (ext.length() > filename.length()) {
    return false;
  }
  return std::equal(ext.rbegin(), ext.rend(), filename.rbegin());
}

int main(int argc, char *argv[]) {
  std::string source;
  std::string filepath = "source.lin";

  if (argc > 1) {
    filepath = argv[1];

    // Check if the file has .lin extension
    if (!hasExtension(filepath, ".lin")) {
      std::cerr << "Error: Dotlin only supports .lin files" << std::endl;
      return 1;
    }

    try {
      source = readFile(filepath);
    } catch (const std::exception &e) {
      std::cerr << "Error reading file: " << e.what() << std::endl;
      return 1;
    }
  } else {
    // Default sample code if no file is provided
    source = "val x = 42\nprintln(x)"; // Sample Kotlin-like syntax
  }

  try {
    auto tokens = dotlin::tokenize(source);
    auto program = dotlin::parse(tokens);

    // Extract command-line arguments (excluding program name and input file)
    std::vector<std::string> cmdArgs;
    for (int i = 2; i < argc; ++i) {
      cmdArgs.push_back(argv[i]);
    }

    // Pass command-line arguments to the interpreter
    auto result = dotlin::interpret(program, cmdArgs, filepath);
    std::cout << "Execution result: " << std::endl;
    // Note: result printing depends on the Value variant implementation
  } catch (const dotlin::DotlinError &e) {
    std::cerr << e.fullMessage() << std::endl;
    return 1;
  } catch (const std::exception &e) {
    std::cerr << "Runtime error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}