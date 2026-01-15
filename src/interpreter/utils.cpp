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

bool valuesEqual(const Value &v1, const Value &v2) {
  if (v1.index() != v2.index()) {
    // Special case: int vs int64_t comparison
    if (std::holds_alternative<int>(v1) &&
        std::holds_alternative<int64_t>(v2)) {
      return static_cast<int64_t>(std::get<int>(v1)) == std::get<int64_t>(v2);
    }
    if (std::holds_alternative<int64_t>(v1) &&
        std::holds_alternative<int>(v2)) {
      return std::get<int64_t>(v1) == static_cast<int64_t>(std::get<int>(v2));
    }
    // int/long vs double
    if ((std::holds_alternative<int>(v1) ||
         std::holds_alternative<int64_t>(v1)) &&
        std::holds_alternative<double>(v2)) {
      double d1 = std::holds_alternative<int>(v1)
                      ? static_cast<double>(std::get<int>(v1))
                      : static_cast<double>(std::get<int64_t>(v1));
      return d1 == std::get<double>(v2);
    }
    if (std::holds_alternative<double>(v1) &&
        (std::holds_alternative<int>(v2) ||
         std::holds_alternative<int64_t>(v2))) {
      double d2 = std::holds_alternative<int>(v2)
                      ? static_cast<double>(std::get<int>(v2))
                      : static_cast<double>(std::get<int64_t>(v2));
      return std::get<double>(v1) == d2;
    }
    return false;
  }

  return std::visit(
      [&v2](auto &&arg1) -> bool {
        using T = std::decay_t<decltype(arg1)>;
        const T &arg2 = std::get<T>(v2);

        if constexpr (std::is_same_v<T, ArrayValue>) {
          if (arg1.elements->size() != arg2.elements->size())
            return false;
          for (size_t i = 0; i < arg1.elements->size(); ++i) {
            if (!valuesEqual((*arg1.elements)[i], (*arg2.elements)[i]))
              return false;
          }
          return true;
        } else {
          return arg1 == arg2;
        }
      },
      v1);
}

} // namespace dotlin
