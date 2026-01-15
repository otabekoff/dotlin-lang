#include "dotlin/interpreter.h"
// #include <iostream>
// #include <stdexcept>

namespace dotlin {

std::string valueToString(const Value &value);
std::string typeToString(const std::shared_ptr<Type> &type);

std::string Interpreter::valueToString(const Value &value) {
  return dotlin::valueToString(value);
}

std::string Interpreter::typeToString(const std::shared_ptr<Type> &type) {
  return dotlin::typeToString(type);
}

} // namespace dotlin
