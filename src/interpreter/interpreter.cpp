#include "dotlin/interpreter.h"
// #include <iostream>
// #include <stdexcept>

namespace dotlin {

// Missing methods that are referenced by other files

std::string Interpreter::valueToString(const Value &value) {
  // Use the global function from utils.cpp
  if (std::holds_alternative<int>(value)) {
    return std::to_string(std::get<int>(value));
  } else if (std::holds_alternative<double>(value)) {
    return std::to_string(std::get<double>(value));
  } else if (std::holds_alternative<bool>(value)) {
    return std::get<bool>(value) ? "true" : "false";
  } else if (std::holds_alternative<std::string>(value)) {
    return std::get<std::string>(value);
  } else if (std::holds_alternative<ArrayValue>(value)) {
    auto array = std::get<ArrayValue>(value);
    std::string result = "[";
    for (size_t i = 0; i < array.elements->size(); ++i) {
      if (i > 0)
        result += ", ";
      result += valueToString((*array.elements)[i]);
    }
    result += "]";
    return result;
  } else if (std::holds_alternative<std::shared_ptr<LambdaValue>>(value)) {
    return "<lambda>";
  } else if (std::holds_alternative<std::shared_ptr<ClassInstance>>(value)) {
    auto instance = std::get<std::shared_ptr<ClassInstance>>(value);
    return instance->className + " instance";
  } else if (std::holds_alternative<std::shared_ptr<ClassDefinition>>(value)) {
    auto classDef = std::get<std::shared_ptr<ClassDefinition>>(value);
    return classDef->name + " class";
  }
  return "null";
}

std::string Interpreter::typeToString(const std::shared_ptr<Type> &type) {
  // Use the global function from utils.cpp
  if (!type)
    return "unknown";
  switch (type->kind) {
  case TypeKind::INT:
    return "Int";
  case TypeKind::DOUBLE:
    return "Double";
  case TypeKind::BOOL:
    return "Boolean";
  case TypeKind::STRING:
    return "String";
  case TypeKind::ARRAY:
    return "Array";
  case TypeKind::UNKNOWN:
    return "Unknown";
  default:
    return "Unknown";
  }
}

} // namespace dotlin
