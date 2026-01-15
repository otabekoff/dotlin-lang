#include "dotlin/interpreter.h"
#include <string>
#include <variant>

namespace dotlin {

std::string getTypeOfValue(const Value &value) {
  return std::visit(
      [](auto &&arg) -> std::string {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, int>)
          return "int";
        else if constexpr (std::is_same_v<T, int64_t>)
          return "long";
        else if constexpr (std::is_same_v<T, double>)
          return "double";
        else if constexpr (std::is_same_v<T, bool>)
          return "bool";
        else if constexpr (std::is_same_v<T, std::string>)
          return "string";
        else if constexpr (std::is_same_v<T, ArrayValue>)
          return "Array";
        else if constexpr (std::is_same_v<T, std::shared_ptr<LambdaValue>>)
          return "Lambda";
        else if constexpr (std::is_same_v<T, std::shared_ptr<ClassInstance>>)
          return "Object";
        else if constexpr (std::is_same_v<T, std::shared_ptr<ClassDefinition>>)
          return "Class";
        else
          return "unknown";
      },
      value);
}

std::string valueToString(const Value &value) {
  return std::visit(
      [](auto &&arg) -> std::string {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_integral_v<T> && !std::is_same_v<T, bool>)
          return std::to_string(arg);
        else if constexpr (std::is_floating_point_v<T>)
          return std::to_string(arg);
        else if constexpr (std::is_same_v<T, bool>)
          return arg ? "true" : "false";
        else if constexpr (std::is_same_v<T, std::string>)
          return arg;
        else if constexpr (std::is_same_v<T, ArrayValue>) {
          std::string result = "[";
          for (size_t i = 0; i < arg.elements->size(); ++i) {
            if (i > 0)
              result += ", ";
            result += valueToString((*arg.elements)[i]);
          }
          result += "]";
          return result;
        } else if constexpr (std::is_same_v<T, std::shared_ptr<LambdaValue>>)
          return "<lambda>";
        else if constexpr (std::is_same_v<T, std::shared_ptr<ClassInstance>>)
          return arg->className + " instance";
        else if constexpr (std::is_same_v<T, std::shared_ptr<ClassDefinition>>)
          return arg->name + " class";
        else
          return "null";
      },
      value);
}

std::string typeToString(const std::shared_ptr<Type> &type) {
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
