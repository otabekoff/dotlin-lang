#include "dotlin/interpreter.h"
#include "dotlin/parser.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
// #include <sstream>

using namespace dotlin;

Value Interpreter::executeBuiltinFunction(
    const std::string &name,
    const std::vector<std::shared_ptr<Expression>> &arguments) {
  // I/O functions
  if (name == "println") {
    if (arguments.empty()) {
      std::cout << std::endl;
      return Value();
    }
    for (size_t i = 0; i < arguments.size(); ++i) {
      Value arg = evaluate(*arguments[i]);
      std::cout << valueToString(arg);
      if (i == arguments.size() - 1) {
        std::cout << std::endl;
      } else {
        std::cout << " ";
      }
    }
    return Value();
  }

  if (name == "print") {
    for (const auto &arg : arguments) {
      Value argValue = evaluate(*arg);
      std::cout << valueToString(argValue);
    }
    return Value();
  }

  if (name == "readln") {
    std::string input;
    std::getline(std::cin, input);
    return Value(input);
  }

  // Mathematical functions
  if (name == "sqrt") {
    if (arguments.size() != 1) {
      throw std::runtime_error("sqrt() expects exactly 1 argument");
    }
    Value arg = evaluate(*arguments[0]);
    if (auto *num = std::get_if<int>(&arg)) {
      if (*num < 0) {
        throw std::runtime_error("sqrt() cannot take negative numbers");
      }
      return Value(static_cast<int>(std::sqrt(*num)));
    }
    throw std::runtime_error("sqrt() expects a number");
  }

  if (name == "abs") {
    if (arguments.size() != 1) {
      throw std::runtime_error("abs() expects exactly 1 argument");
    }
    Value arg = evaluate(*arguments[0]);
    if (auto *num = std::get_if<int>(&arg)) {
      return Value(std::abs(*num));
    }
    throw std::runtime_error("abs() expects a number");
  }

  if (name == "pow") {
    if (arguments.size() != 2) {
      throw std::runtime_error("pow() expects exactly 2 arguments");
    }
    Value base = evaluate(*arguments[0]);
    Value exp = evaluate(*arguments[1]);
    if (auto *baseNum = std::get_if<int>(&base)) {
      if (auto *expNum = std::get_if<int>(&exp)) {
        return Value(static_cast<int>(std::pow(*baseNum, *expNum)));
      }
    }
    throw std::runtime_error("pow() expects numbers");
  }

  if (name == "sin") {
    if (arguments.size() != 1)
      throw std::runtime_error("sin() expects 1 argument");
    Value arg = evaluate(*arguments[0]);
    if (auto *num = std::get_if<double>(&arg))
      return Value(std::sin(*num));
    if (auto *num = std::get_if<int>(&arg))
      return Value(std::sin(*num));
    throw std::runtime_error("sin() expects a number");
  }
  if (name == "cos") {
    if (arguments.size() != 1)
      throw std::runtime_error("cos() expects 1 argument");
    Value arg = evaluate(*arguments[0]);
    if (auto *num = std::get_if<double>(&arg))
      return Value(std::cos(*num));
    if (auto *num = std::get_if<int>(&arg))
      return Value(std::cos(*num));
    throw std::runtime_error("cos() expects a number");
  }
  if (name == "tan") {
    if (arguments.size() != 1)
      throw std::runtime_error("tan() expects 1 argument");
    Value arg = evaluate(*arguments[0]);
    if (auto *num = std::get_if<double>(&arg))
      return Value(std::tan(*num));
    if (auto *num = std::get_if<int>(&arg))
      return Value(std::tan(*num));
    throw std::runtime_error("tan() expects a number");
  }

  if (name == "min") {
    if (arguments.size() != 2)
      throw std::runtime_error("min() expects 2 arguments");
    Value a = evaluate(*arguments[0]);
    Value b = evaluate(*arguments[1]);
    if (std::holds_alternative<int>(a) && std::holds_alternative<int>(b)) {
      return Value(std::min(std::get<int>(a), std::get<int>(b)));
    }
    if (std::holds_alternative<double>(a) ||
        std::holds_alternative<double>(b)) {
      double da = std::holds_alternative<int>(a) ? std::get<int>(a)
                                                 : std::get<double>(a);
      double db = std::holds_alternative<int>(b) ? std::get<int>(b)
                                                 : std::get<double>(b);
      return Value(std::min(da, db));
    }
    throw std::runtime_error("min() expects numbers");
  }
  if (name == "max") {
    if (arguments.size() != 2)
      throw std::runtime_error("max() expects 2 arguments");
    Value a = evaluate(*arguments[0]);
    Value b = evaluate(*arguments[1]);
    if (std::holds_alternative<int>(a) && std::holds_alternative<int>(b)) {
      return Value(std::max(std::get<int>(a), std::get<int>(b)));
    }
    if (std::holds_alternative<double>(a) ||
        std::holds_alternative<double>(b)) {
      double da = std::holds_alternative<int>(a) ? std::get<int>(a)
                                                 : std::get<double>(a);
      double db = std::holds_alternative<int>(b) ? std::get<int>(b)
                                                 : std::get<double>(b);
      return Value(std::max(da, db));
    }
    throw std::runtime_error("max() expects numbers");
  }

  if (name == "round") {
    if (arguments.size() != 1)
      throw std::runtime_error("round() expects 1 argument");
    Value arg = evaluate(*arguments[0]);
    if (auto *num = std::get_if<double>(&arg))
      return Value(static_cast<int>(std::round(*num)));
    if (auto *num = std::get_if<int>(&arg))
      return Value(*num);
    throw std::runtime_error("round() expects a number");
  }
  if (name == "ceil") {
    if (arguments.size() != 1)
      throw std::runtime_error("ceil() expects 1 argument");
    Value arg = evaluate(*arguments[0]);
    if (auto *num = std::get_if<double>(&arg))
      return Value(static_cast<int>(std::ceil(*num)));
    if (auto *num = std::get_if<int>(&arg))
      return Value(*num);
    throw std::runtime_error("ceil() expects a number");
  }
  if (name == "floor") {
    if (arguments.size() != 1)
      throw std::runtime_error("floor() expects 1 argument");
    Value arg = evaluate(*arguments[0]);
    if (auto *num = std::get_if<double>(&arg))
      return Value(static_cast<int>(std::floor(*num)));
    if (auto *num = std::get_if<int>(&arg))
      return Value(*num);
    throw std::runtime_error("floor() expects a number");
  }

  if (name == "random") {
    if (arguments.size() != 0)
      throw std::runtime_error("random() expects 0 arguments");
    return Value(static_cast<double>(std::rand()) / RAND_MAX);
  }

  // String functions
  if (name == "length") {
    if (arguments.size() != 1) {
      throw std::runtime_error("length() expects exactly 1 argument");
    }
    Value arg = evaluate(*arguments[0]);
    if (auto *str = std::get_if<std::string>(&arg)) {
      return Value(static_cast<int>(str->length()));
    }
    if (auto *array = std::get_if<ArrayValue>(&arg)) {
      return Value(static_cast<int>(array->elements->size()));
    }
    throw std::runtime_error("length() expects a string or array");
  }

  if (name == "substring") {
    if (arguments.size() != 3) {
      throw std::runtime_error("substring() expects exactly 3 arguments");
    }
    Value str = evaluate(*arguments[0]);
    Value start = evaluate(*arguments[1]);
    Value end = evaluate(*arguments[2]);

    if (auto *strVal = std::get_if<std::string>(&str)) {
      if (auto *startVal = std::get_if<int>(&start)) {
        if (auto *endVal = std::get_if<int>(&end)) {
          if (*startVal < 0 || *endVal > static_cast<int>(strVal->length()) ||
              *startVal > *endVal) {
            throw std::runtime_error("Invalid substring indices");
          }
          return Value(
              strVal->substr(static_cast<size_t>(*startVal),
                             static_cast<size_t>(*endVal - *startVal)));
        }
      }
    }
    throw std::runtime_error("substring() expects (string, int, int)");
  }

  if (name == "toUpperCase") {
    if (arguments.size() != 1) {
      throw std::runtime_error("toUpperCase() expects exactly 1 argument");
    }
    Value arg = evaluate(*arguments[0]);
    if (auto *str = std::get_if<std::string>(&arg)) {
      std::string upperStr = *str;
      std::transform(upperStr.begin(), upperStr.end(), upperStr.begin(),
                     ::toupper);
      return Value(upperStr);
    }
    throw std::runtime_error("toUpperCase() expects a string");
  }

  if (name == "toLowerCase") {
    if (arguments.size() != 1) {
      throw std::runtime_error("toLowerCase() expects exactly 1 argument");
    }
    Value arg = evaluate(*arguments[0]);
    if (auto *str = std::get_if<std::string>(&arg)) {
      std::string lowerStr = *str;
      std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(),
                     ::tolower);
      return Value(lowerStr);
    }
    throw std::runtime_error("toLowerCase() expects a string");
  }

  // Array functions
  if (name == "arrayOf") {
    ArrayValue array;
    for (const auto &arg : arguments) {
      array.elements->push_back(evaluate(*arg));
    }
    return Value(array);
  }

  if (name == "indexOf") {
    if (arguments.size() != 2) {
      throw std::runtime_error("indexOf() expects exactly 2 arguments");
    }
    Value array = evaluate(*arguments[0]);
    Value element = evaluate(*arguments[1]);

    if (auto *arrayVal = std::get_if<ArrayValue>(&array)) {
      for (size_t i = 0; i < arrayVal->elements->size(); ++i) {
        // Simple equality check for now
        if ((*arrayVal->elements)[i].index() == element.index()) {
          if ((*arrayVal->elements)[i] == element) {
            return Value(static_cast<int>(i));
          }
        }
      }
      return Value(-1); // Not found
    }
    throw std::runtime_error("indexOf() expects an array");
  }

  // Type checking functions
  if (name == "isString") {
    if (arguments.size() != 1) {
      throw std::runtime_error("isString() expects exactly 1 argument");
    }
    Value arg = evaluate(*arguments[0]);
    return Value(std::holds_alternative<std::string>(arg));
  }

  if (name == "isInt") {
    if (arguments.size() != 1) {
      throw std::runtime_error("isInt() expects exactly 1 argument");
    }
    Value arg = evaluate(*arguments[0]);
    return Value(std::holds_alternative<int>(arg));
  }

  if (name == "isBoolean") {
    if (arguments.size() != 1) {
      throw std::runtime_error("isBoolean() expects exactly 1 argument");
    }
    Value arg = evaluate(*arguments[0]);
    return Value(std::holds_alternative<bool>(arg));
  }

  if (name == "isArray") {
    if (arguments.size() != 1) {
      throw std::runtime_error("isArray() expects exactly 1 argument");
    }
    Value arg = evaluate(*arguments[0]);
    return Value(std::holds_alternative<ArrayValue>(arg));
  }

  // Conversion functions
  if (name == "toString") {
    if (arguments.size() != 1) {
      throw std::runtime_error("toString() expects exactly 1 argument");
    }
    Value arg = evaluate(*arguments[0]);
    return Value(this->valueToString(arg));
  }

  if (name == "toInt") {
    if (arguments.size() != 1) {
      throw std::runtime_error("toInt() expects exactly 1 argument");
    }
    Value arg = evaluate(*arguments[0]);
    if (auto *str = std::get_if<std::string>(&arg)) {
      try {
        return Value(std::stoi(*str));
      } catch (...) {
        throw std::runtime_error("Cannot convert string to int");
      }
    }
    if (auto *num = std::get_if<int>(&arg)) {
      return Value(*num);
    }
    throw std::runtime_error("toInt() expects a string or number");
  }

  // I/O functions
  if (name == "readLine") {
    if (arguments.size() > 1) {
      throw std::runtime_error("readLine() expects at most 1 argument");
    }

    std::string prompt = "";
    if (arguments.size() == 1) {
      Value promptArg = evaluate(*arguments[0]);
      if (auto *promptStr = std::get_if<std::string>(&promptArg)) {
        prompt = *promptStr;
      }
    }

    std::cout << prompt;
    std::string input;
    std::getline(std::cin, input);
    return Value(input);
  }

  // System functions
  if (name == "exit") {
    if (arguments.size() != 1) {
      throw std::runtime_error("exit() expects exactly 1 argument");
    }
    Value code = evaluate(*arguments[0]);
    if (auto *codeInt = std::get_if<int>(&code)) {
      std::exit(*codeInt);
    }
    throw std::runtime_error("exit() expects an integer");
  }

  if (name == "clock") {
    if (arguments.size() != 0) {
      throw std::runtime_error("clock() expects no arguments");
    }
    // Return current time in milliseconds
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto millis =
        std::chrono::duration_cast<std::chrono::milliseconds>(duration);
    return Value(static_cast<int>(millis.count()));
  }

  if (name == "format") {
    if (arguments.empty()) {
      throw std::runtime_error(
          "format function requires at least one argument (format string)");
    }

    Value firstArg = evaluate(*arguments[0]);
    if (!std::holds_alternative<std::string>(firstArg)) {
      throw std::runtime_error("First argument to format must be a string");
    }

    std::string formatStr = std::get<std::string>(firstArg);
    std::string result = "";
    size_t argIndex = 1;

    for (size_t i = 0; i < formatStr.length(); ++i) {
      if (formatStr[i] == '%' && i + 1 < formatStr.length()) {
        char specifier = formatStr[i + 1];
        if (specifier == '%') {
          result += '%';
          i++;
        } else if (argIndex < arguments.size()) {
          Value val = evaluate(*arguments[argIndex++]);
          if (specifier == 'd') {
            if (std::holds_alternative<int>(val))
              result += std::to_string(std::get<int>(val));
            else if (std::holds_alternative<double>(val))
              result += std::to_string(static_cast<int>(std::get<double>(val)));
            else
              result += "0";
          } else if (specifier == 'f') {
            if (std::holds_alternative<double>(val))
              result += std::to_string(std::get<double>(val));
            else if (std::holds_alternative<int>(val))
              result += std::to_string(static_cast<double>(std::get<int>(val)));
            else
              result += "0.0";
          } else if (specifier == 's') {
            result += valueToString(val);
          } else {
            result += valueToString(val);
          }
          i++;
        } else {
          result += formatStr[i];
        }
      } else {
        result += formatStr[i];
      }
    }
    return Value(result);
  }

  throw std::runtime_error("Unknown built-in function: " + name);
}
