#include "dotlin/interpreter.h"
// #include <iostream>
#include <variant>

namespace dotlin {

std::string getTypeOfValue(const Value &value) {
  if (std::holds_alternative<int>(value)) {
    return "int";
  } else if (std::holds_alternative<double>(value)) {
    return "double";
  } else if (std::holds_alternative<bool>(value)) {
    return "bool";
  } else if (std::holds_alternative<std::string>(value)) {
    return "string";
  } else if (std::holds_alternative<ArrayValue>(value)) {
    return "Array";
  } else if (std::holds_alternative<std::shared_ptr<LambdaValue>>(value)) {
    return "Lambda";
  } else if (std::holds_alternative<std::shared_ptr<ClassInstance>>(value)) {
    return "Object";
  } else if (std::holds_alternative<std::shared_ptr<ClassDefinition>>(value)) {
    return "Class";
  }
  return "unknown";
}

std::string valueToString(const Value &value) {
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
    for (size_t i = 0; i < array.elements.size(); ++i) {
      if (i > 0)
        result += ", ";
      result += valueToString(array.elements[i]);
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

} // namespace dotlin
