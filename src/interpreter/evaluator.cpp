#include "dotlin/interpreter.h"
#include "dotlin/parser.h"
#include "dotlin/visitors.h"
#include <algorithm>
#include <cmath>
#include <iostream>
// #include <algorithm>
// #include <cmath>
#include <stdexcept>

// Forward declaration of valueToString
namespace dotlin {
std::string valueToString(const Value &value);
}

using namespace dotlin;

// Expression evaluation visitor implementations
void EvalVisitor::visit(LiteralExpr &node) {
  std::visit(
      [this](auto &&arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, int>) {
          result = Value(arg);
        } else if constexpr (std::is_same_v<T, double>) {
          result = Value(arg);
        } else if constexpr (std::is_same_v<T, bool>) {
          result = Value(arg);
        } else if constexpr (std::is_same_v<T, std::string>) {
          result = Value(arg);
        }
      },
      node.value);
}

void EvalVisitor::visit(StringInterpolationExpr &node) {
  std::string resultStr = "";
  for (const auto &part : node.parts) {
    Value partValue = interpreter->evaluate(*part);
    resultStr += interpreter->valueToString(partValue);
  }
  result = Value(resultStr);
}

void EvalVisitor::visit(IdentifierExpr &node) {
  // Check if this is a special identifier like "args"
  if (node.name == "args") {
    // Return the command-line arguments array
    ArrayValue argsArray;
    for (const auto &arg : interpreter->commandLineArgs) {
      argsArray.elements->push_back(Value(arg));
    }
    result = Value(argsArray);
    return;
  }
  // Special handling for "this" keyword
  else if (node.name == "this") {
    try {
      result = interpreter->environment->get("this");
    } catch (const std::exception &e) {
      result = Value(std::string("undefined"));
    }
  }
  // Look up the value in the environment
  else {
    try {
      result = interpreter->environment->get(node.name);
    } catch (const std::runtime_error &e) {
      // Check if this is a built-in function
      if (node.name == "println" || node.name == "print" ||
          node.name == "sqrt" || node.name == "abs" || node.name == "pow" ||
          node.name == "readln" || node.name == "arrayOf") {
        // Return a special lambda that represents a built-in function
        auto builtinLambda =
            std::make_shared<LambdaValue>(std::vector<FunctionParameter>(),
                                          nullptr, interpreter->environment);
        result = Value(builtinLambda);
      } else {
        result = Value(std::string("undefined"));
      }
    }
  }
}

void EvalVisitor::visit(LambdaExpr &node) {
  // Create a lambda value
  auto lambda = std::make_shared<LambdaValue>(node.parameters, node.body,
                                              interpreter->environment, &node);
  result = Value(lambda);
}

void EvalVisitor::visit(BinaryExpr &node) {
  if (node.op == TokenType::ASSIGN) {
    // Handle assignment
    if (auto *ident = dynamic_cast<IdentifierExpr *>(node.left.get())) {
      Value value = interpreter->evaluate(*node.right);
      interpreter->environment->assign(ident->name, value);
      result = value;
      return;
    }
    // Handle member access assignment (e.g., this.name = value)
    else if (auto *memberAccess =
                 dynamic_cast<MemberAccessExpr *>(node.left.get())) {
      Value value = interpreter->evaluate(*node.right);
      Value objValue = interpreter->evaluate(*memberAccess->object);

      if (auto *instance =
              std::get_if<std::shared_ptr<ClassInstance>>(&objValue)) {
        // Assign to the field
        (*instance)->fields[memberAccess->property] = value;
        result = value;
        return;
      }
      throw std::runtime_error("Cannot assign to non-object field");
    }
    throw std::runtime_error("Invalid assignment target");
  }

  // Handle other binary operations
  Value left = interpreter->evaluate(*node.left);
  Value right = interpreter->evaluate(*node.right);

  // Handle arithmetic operations
  if (node.op == TokenType::PLUS) {
    if (auto *leftInt = std::get_if<int>(&left)) {
      if (auto *rightInt = std::get_if<int>(&right)) {
        result = Value(*leftInt + *rightInt);
        return;
      }
    }
    // Handle string concatenation with type conversion
    if (auto *leftStr = std::get_if<std::string>(&left)) {
      result = Value(*leftStr + interpreter->valueToString(right));
      return;
    }
    if (auto *rightStr = std::get_if<std::string>(&right)) {
      result = Value(interpreter->valueToString(left) + *rightStr);
      return;
    }
    throw std::runtime_error("Invalid operands for + operator");
  }

  if (node.op == TokenType::MINUS) {
    if (auto *leftInt = std::get_if<int>(&left)) {
      if (auto *rightInt = std::get_if<int>(&right)) {
        result = Value(*leftInt - *rightInt);
        return;
      }
    }
    throw std::runtime_error("Invalid operands for - operator");
  }

  if (node.op == TokenType::MULTIPLY) {
    if (auto *leftInt = std::get_if<int>(&left)) {
      if (auto *rightInt = std::get_if<int>(&right)) {
        result = Value(*leftInt * *rightInt);
        return;
      }
    }
    throw std::runtime_error("Invalid operands for * operator");
  }

  if (node.op == TokenType::DIVIDE) {
    if (auto *leftInt = std::get_if<int>(&left)) {
      if (auto *rightInt = std::get_if<int>(&right)) {
        if (*rightInt == 0) {
          throw std::runtime_error("Division by zero");
        }
        result = Value(*leftInt / *rightInt);
        return;
      }
    }
    throw std::runtime_error("Invalid operands for / operator");
  }

  // Handle comparison operations
  if (node.op == TokenType::EQUAL) {
    bool isEqual = false;
    if (left.index() != right.index()) {
      // Different types are not equal
      result = Value(isEqual);
      return;
    }

    if (auto *leftInt = std::get_if<int>(&left)) {
      if (auto *rightInt = std::get_if<int>(&right)) {
        isEqual = (*leftInt == *rightInt);
      }
    } else if (auto *leftStr = std::get_if<std::string>(&left)) {
      if (auto *rightStr = std::get_if<std::string>(&right)) {
        isEqual = (*leftStr == *rightStr);
      }
    } else if (auto *leftBool = std::get_if<bool>(&left)) {
      if (auto *rightBool = std::get_if<bool>(&right)) {
        isEqual = (*leftBool == *rightBool);
      }
    }
    result = Value(isEqual);
    return;
  }

  if (node.op == TokenType::NOT_EQUAL) {
    bool isEqual = false;
    if (left.index() != right.index()) {
      // Different types are not equal
      result = Value(true);
      return;
    }

    if (auto *leftInt = std::get_if<int>(&left)) {
      if (auto *rightInt = std::get_if<int>(&right)) {
        isEqual = (*leftInt == *rightInt);
      }
    } else if (auto *leftStr = std::get_if<std::string>(&left)) {
      if (auto *rightStr = std::get_if<std::string>(&right)) {
        isEqual = (*leftStr == *rightStr);
      }
    } else if (auto *leftBool = std::get_if<bool>(&left)) {
      if (auto *rightBool = std::get_if<bool>(&right)) {
        isEqual = (*leftBool == *rightBool);
      }
    }
    result = Value(!isEqual);
    return;
  }

  if (node.op == TokenType::LESS) {
    if (auto *leftInt = std::get_if<int>(&left)) {
      if (auto *rightInt = std::get_if<int>(&right)) {
        result = Value(*leftInt < *rightInt);
        return;
      }
    }
    throw std::runtime_error("Invalid operands for < operator");
  }

  if (node.op == TokenType::LESS_EQUAL) {
    if (auto *leftInt = std::get_if<int>(&left)) {
      if (auto *rightInt = std::get_if<int>(&right)) {
        result = Value(*leftInt <= *rightInt);
        return;
      }
    }
    throw std::runtime_error("Invalid operands for <= operator");
  }

  if (node.op == TokenType::GREATER) {
    if (auto *leftInt = std::get_if<int>(&left)) {
      if (auto *rightInt = std::get_if<int>(&right)) {
        result = Value(*leftInt > *rightInt);
        return;
      }
    }
    throw std::runtime_error("Invalid operands for > operator");
  }

  if (node.op == TokenType::GREATER_EQUAL) {
    if (auto *leftInt = std::get_if<int>(&left)) {
      if (auto *rightInt = std::get_if<int>(&right)) {
        result = Value(*leftInt >= *rightInt);
        return;
      }
    }
    throw std::runtime_error("Invalid operands for >= operator");
  }

  throw std::runtime_error("Unknown binary operator");
}

void EvalVisitor::visit(UnaryExpr &node) {
  Value operand = interpreter->evaluate(*node.operand);

  if (node.op == TokenType::MINUS) {
    if (auto *intValue = std::get_if<int>(&operand)) {
      result = Value(-*intValue);
      return;
    }
    throw std::runtime_error("Invalid operand for unary minus");
  }

  if (node.op == TokenType::NOT) {
    if (auto *boolValue = std::get_if<bool>(&operand)) {
      result = Value(!*boolValue);
      return;
    }
    throw std::runtime_error("Invalid operand for logical not");
  }

  throw std::runtime_error("Unknown unary operator");
}

void EvalVisitor::visit(CallExpr &node) {
  // Check if this is a method call on a class instance or built-in type
  if (auto *memberAccess =
          dynamic_cast<MemberAccessExpr *>(node.callee.get())) {
    // Handle method calls like obj.method()
    auto objValue = interpreter->evaluate(*memberAccess->object);
    std::string methodName = memberAccess->property;

    // Evaluate arguments first (needed for string methods)
    std::vector<Value> args;
    for (const auto &arg : node.arguments) {
      if (arg) {
        args.push_back(interpreter->evaluate(*arg));
      }
    }

    // Handle method calls on built-in types (strings, arrays, etc.)
    if (methodName == "toString" && args.empty()) {
      result = Value(interpreter->valueToString(objValue));
      return;
    } else if (auto *array = std::get_if<ArrayValue>(&objValue)) {
      if (methodName == "size" && args.empty()) {
        result = Value(static_cast<int>(array->elements->size()));
        return;
      } else if (methodName == "contentToString" && args.empty()) {
        std::string content = "[";
        for (size_t i = 0; i < array->elements->size(); ++i) {
          if (i > 0) {
            content += ", ";
          }
          content += interpreter->valueToString((*array->elements)[i]);
        }
        content += "]";
        result = Value(content);
        return;
      } else if (methodName == "add") {
        if (args.size() == 1) {
          array->push_back(args[0]);
          result = Value(true);
          return;
        } else {
          throw std::runtime_error("add method requires 1 argument");
        }
      } else if (methodName == "get") {
        if (args.size() == 1 && std::holds_alternative<int>(args[0])) {
          int index = std::get<int>(args[0]);
          result = array->get(static_cast<size_t>(index));
          return;
        } else {
          throw std::runtime_error("get method requires 1 integer argument");
        }
      } else if (methodName == "set") {
        if (args.size() == 2 && std::holds_alternative<int>(args[0])) {
          int index = std::get<int>(args[0]);
          array->set(static_cast<size_t>(index), args[1]);
          result = args[1]; // Return the assigned value
          return;
        } else {
          throw std::runtime_error("set method requires index and value");
        }
      } else if (methodName == "removeAt") {
        if (args.size() == 1 && std::holds_alternative<int>(args[0])) {
          int index = std::get<int>(args[0]);
          array->removeAt(static_cast<size_t>(index));
          result = Value(true);
          return;
        } else {
          throw std::runtime_error(
              "removeAt method requires 1 integer argument");
        }
      }
    } else if (methodName == "substring" && args.size() >= 1) {
      // Handle substring method calls
      if (auto *strValue = std::get_if<std::string>(&objValue)) {
        if (args.size() == 1 && std::holds_alternative<int>(args[0])) {
          std::string str = *strValue;
          int start = std::get<int>(args[0]);
          if (start >= 0 && static_cast<size_t>(start) <= str.length()) {
            result = Value(str.substr(static_cast<size_t>(start)));
          } else {
            result = Value(std::string(""));
          }
        } else if (args.size() == 2 && std::holds_alternative<int>(args[0]) &&
                   std::holds_alternative<int>(args[1])) {
          std::string str = *strValue;
          int start = std::get<int>(args[0]);
          int end = std::get<int>(args[1]);
          if (start >= 0 && static_cast<size_t>(end) <= str.length() &&
              start <= end) {
            result = Value(str.substr(static_cast<size_t>(start),
                                      static_cast<size_t>(end - start)));
          } else {
            result = Value(std::string(""));
          }
        }
      }
      return;
    } else if (methodName == "indexOf" && args.size() == 1 &&
               std::holds_alternative<std::string>(args[0])) {
      // Handle indexOf method calls
      if (auto *strValue = std::get_if<std::string>(&objValue)) {
        std::string str = *strValue;
        std::string substr = std::get<std::string>(args[0]);
        size_t pos = str.find(substr);
        result = Value(static_cast<int>(
            pos != std::string::npos ? static_cast<int>(pos) : -1));
      } else {
        result = Value(-1);
      }
      return;
    } else if (methodName == "startsWith" && args.size() == 1 &&
               std::holds_alternative<std::string>(args[0])) {
      // Handle startsWith method calls
      if (auto *strValue = std::get_if<std::string>(&objValue)) {
        std::string str = *strValue;
        std::string prefix = std::get<std::string>(args[0]);
        result =
            Value(static_cast<bool>(str.substr(0, prefix.length()) == prefix));
      } else {
        result = Value(false);
      }
      return;
    } else if (methodName == "endsWith" && args.size() == 1 &&
               std::holds_alternative<std::string>(args[0])) {
      // Handle endsWith method calls
      if (auto *strValue = std::get_if<std::string>(&objValue)) {
        std::string str = *strValue;
        std::string suffix = std::get<std::string>(args[0]);
        if (str.length() >= suffix.length()) {
          result =
              Value(static_cast<bool>(str.substr(str.length() - suffix.length(),
                                                 suffix.length()) == suffix));
        } else {
          result = Value(false);
        }
      } else {
        result = Value(false);
      }
      return;
    } else if (methodName == "toUpperCase" && args.empty()) {
      // Handle toUpperCase method calls
      if (auto *strValue = std::get_if<std::string>(&objValue)) {
        std::string str = *strValue;
        std::transform(str.begin(), str.end(), str.begin(), ::toupper);
        result = Value(str);
      } else {
        result = Value(std::string(""));
      }
      return;
    } else if (methodName == "toLowerCase" && args.empty()) {
      // Handle toLowerCase method calls
      if (auto *strValue = std::get_if<std::string>(&objValue)) {
        std::string str = *strValue;
        std::transform(str.begin(), str.end(), str.begin(), ::tolower);
        result = Value(str);
      } else {
        result = Value(std::string(""));
      }
      return;
    } else if (methodName == "trim" && args.empty()) {
      // Handle trim method calls
      if (auto *strValue = std::get_if<std::string>(&objValue)) {
        std::string str = *strValue;
        size_t start = str.find_first_not_of(" \t\n\r\f\v");
        if (start == std::string::npos) {
          result = Value(str);
        } else {
          size_t end = str.find_last_not_of(" \t\n\r\f\v");
          result = Value(str.substr(start, end - start + 1));
        }
      } else {
        result = Value(std::string(""));
      }
      return;
    } else if (methodName == "split" && args.size() == 1 &&
               std::holds_alternative<std::string>(args[0])) {
      // Handle split method calls
      if (auto *strValue = std::get_if<std::string>(&objValue)) {
        std::string str = *strValue;
        std::string delim = std::get<std::string>(args[0]);
        std::vector<Value> parts;
        size_t pos = 0;
        size_t delimLen = delim.length();
        std::string remaining = str;

        while ((pos = remaining.find(delim, pos)) != std::string::npos) {
          parts.push_back(Value(remaining.substr(0, pos)));
          pos += delimLen;
          remaining = remaining.substr(pos);
          pos = 0;
        }
        // Add the remaining part
        parts.push_back(Value(remaining));
        result = Value(ArrayValue(parts));
      } else {
        result = Value(ArrayValue());
      }
      return;
    }

    // Check if the object is a class instance
    if (auto *instance =
            std::get_if<std::shared_ptr<ClassInstance>>(&objValue)) {
      // Look up the method in the class definition
      for (const auto &method : (*instance)->classDef->methods) {
        if (method->name == methodName) {
          // Found the method, execute it
          auto methodEnv =
              std::make_shared<Environment>(interpreter->environment);
          methodEnv->define("this", Value(*instance));

          // Bind method parameters to arguments
          for (size_t i = 0; i < method->parameters.size(); ++i) {
            if (i < node.arguments.size()) {
              Value argValue = interpreter->evaluate(*node.arguments[i]);
              methodEnv->define(method->parameters[i].name, argValue);
            } else {
              methodEnv->define(method->parameters[i].name, Value());
            }
          }

          // Switch to method environment
          interpreter->environment = methodEnv;
          interpreter->functionEnvironment = methodEnv;

          // Execute method body
          Value returnValue =
              interpreter->executeFunction(method->body.get(), methodEnv);
          interpreter->environment = methodEnv->enclosing;
          interpreter->functionEnvironment = methodEnv->enclosing;

          result = returnValue;
          return;
        }
      }
      throw std::runtime_error("Method '" + methodName + "' not found");
    }

    throw std::runtime_error("Cannot call method '" + methodName +
                             "' on this object");
  }

  // Handle regular function calls
  Value calleeValue = interpreter->evaluate(*node.callee);
  if (auto *lambda = std::get_if<std::shared_ptr<LambdaValue>>(&calleeValue)) {
    // Check if this is a built-in function
    if ((*lambda)->parameters.empty() && (*lambda)->body == nullptr) {
      // This is a built-in function - get the function name
      std::string functionName = "";
      if (auto *identifier =
              dynamic_cast<IdentifierExpr *>(node.callee.get())) {
        functionName = identifier->name;
      }

      // Convert unique_ptr to shared_ptr for built-in function call
      std::vector<std::shared_ptr<Expression>> sharedArgs;
      for (const auto &arg : node.arguments) {
        sharedArgs.push_back(
            std::shared_ptr<Expression>(arg.get(), [](Expression *) {}));
      }
      result = interpreter->executeBuiltinFunction(functionName, sharedArgs);
      return;
    }

    // Create new environment for the function
    auto funcEnv = std::make_shared<Environment>((*lambda)->closure);

    // Bind parameters to arguments
    for (size_t i = 0; i < (*lambda)->parameters.size(); ++i) {
      if (i < node.arguments.size()) {
        Value argValue = interpreter->evaluate(*node.arguments[i]);
        funcEnv->define((*lambda)->parameters[i].name, argValue);
      } else {
        // Default parameter value
        funcEnv->define((*lambda)->parameters[i].name, Value());
      }
    }

    // Switch to function environment
    interpreter->environment = funcEnv;
    interpreter->functionEnvironment = funcEnv;

    // Execute function body
    Value returnValue =
        interpreter->executeFunction((*lambda)->body.get(), funcEnv);
    interpreter->environment = funcEnv->enclosing;
    interpreter->functionEnvironment = funcEnv->enclosing;

    result = returnValue;
  } else {
    throw std::runtime_error("Attempt to call a non-function value");
  }
}

void EvalVisitor::visit(MemberAccessExpr &node) {
  if (!node.object) {
    std::cerr << "ERROR: Member access expression has null object" << std::endl;
    return;
  }

  auto objValue = node.object ? interpreter->evaluate(*node.object)
                              : Value(std::string("null"));

  // Check if the object is an identifier "args" and the property is "size"
  // or "contentToString"
  if (auto *identifier = dynamic_cast<IdentifierExpr *>(node.object.get())) {
    if (identifier->name == "args") {
      if (node.property == "size") {
        if (auto *argsArray = std::get_if<ArrayValue>(&objValue)) {
          result = Value(static_cast<int>(argsArray->elements->size()));
          return;
        }
      } else if (node.property == "contentToString") {
        if (auto *argsArray = std::get_if<ArrayValue>(&objValue)) {
          std::string content = "[";
          for (size_t i = 0; i < argsArray->elements->size(); ++i) {
            if (i > 0) {
              content += ", ";
            }
            content += valueToString((*argsArray->elements)[i]);
          }
          content += "]";
          result = Value(content);
          return;
        }
      }
    }
  }

  // Check if the object is a class instance
  if (auto *instance = std::get_if<std::shared_ptr<ClassInstance>>(&objValue)) {
    // Look up the property in the instance's fields
    auto it = (*instance)->fields.find(node.property);
    if (it != (*instance)->fields.end()) {
      result = it->second;
      return;
    }

    // Check if the object is a string and has string properties/methods
    if (auto *strValue = std::get_if<std::string>(&objValue)) {
      if (node.property == "length") {
        result = Value(static_cast<int>(strValue->length()));
        return;
      } else if (node.property == "substring") {
        throw std::runtime_error("Substring method requires arguments");
      } else if (node.property == "indexOf") {
        throw std::runtime_error("IndexOf method requires arguments");
      } else if (node.property == "startsWith") {
        throw std::runtime_error("StartsWith method requires arguments");
      } else if (node.property == "endsWith") {
        throw std::runtime_error("EndsWith method requires arguments");
      } else if (node.property == "toUpperCase") {
        throw std::runtime_error("ToUpper method requires arguments");
      } else if (node.property == "toLowerCase") {
        throw std::runtime_error("ToLower method requires arguments");
      } else if (node.property == "trim") {
        std::string trimmedStr = *strValue;
        size_t start = trimmedStr.find_first_not_of(" \t\n\r\f\v");
        if (start == std::string::npos) {
          result = Value(trimmedStr);
        } else {
          size_t end = trimmedStr.find_last_not_of(" \t\n\r\f\v");
          result = Value(trimmedStr.substr(start, end - start + 1));
        }
        return;
      } else if (node.property == "split") {
        throw std::runtime_error("Split method requires arguments");
      } else if (node.property == "contentToString") {
        std::string content = "[";
        for (size_t i = 0; i < strValue->length(); ++i) {
          if (i > 0) {
            content += ", ";
          }
          content += valueToString(objValue);
        }
        content += "]";
        result = Value(content);
        return;
      }
    } else {
      // For now, just throw an error for unimplemented string properties
      throw std::runtime_error("String property '" + node.property +
                               "' not implemented yet");
    }
  }

  // Check if the object is an array
  if (auto *array = std::get_if<ArrayValue>(&objValue)) {
    if (node.property == "size") {
      result = Value(static_cast<int>(array->elements->size()));
      return;
    }
    if (node.property == "contentToString") {
      std::string content = "[";
      for (size_t i = 0; i < array->elements->size(); ++i) {
        if (i > 0) {
          content += ", ";
        }
        content += valueToString((*array->elements)[i]);
      }
      content += "]";
      result = Value(content);
      return;
    }

    throw std::runtime_error("Invalid member access");
  }
}

void EvalVisitor::visit(ArrayLiteralExpr &node) {
  ArrayValue array;
  for (const auto &element : node.elements) {
    array.elements->push_back(interpreter->evaluate(*element));
  }
  result = Value(array);
}

void EvalVisitor::visit(ArrayAccessExpr &node) {
  Value arrayValue = interpreter->evaluate(*node.array);
  Value indexValue = interpreter->evaluate(*node.index);

  if (auto *array = std::get_if<ArrayValue>(&arrayValue)) {
    if (auto *index = std::get_if<int>(&indexValue)) {
      if (*index >= 0 && *index < static_cast<int>(array->elements->size())) {
        result = (*array->elements)[static_cast<size_t>(*index)];
        return;
      }
      throw std::runtime_error("Array index out of bounds");
    }
    throw std::runtime_error("Array index must be an integer");
  }

  throw std::runtime_error("Invalid array access");
}

// Statement visit methods (needed for complete interface but not used in
// expression evaluation)
void EvalVisitor::visit(ExpressionStmt &node) {
  result = interpreter->evaluate(*node.expression);
}

void EvalVisitor::visit(VariableDeclStmt &node) {
  (void)node;
  result = Value();
}

void EvalVisitor::visit(FunctionDeclStmt &node) {
  (void)node;
  result = Value();
}

void EvalVisitor::visit(BlockStmt &node) {
  (void)node;
  result = Value();
}

void EvalVisitor::visit(IfStmt &node) {
  (void)node;
  result = Value();
}

void EvalVisitor::visit(ReturnStmt &node) {
  (void)node;
  result = Value();
}

void EvalVisitor::visit(ClassDeclStmt &node) {
  (void)node;
  result = Value();
}

void EvalVisitor::visit(ForStmt &node) {
  // Evaluate the iterable expression
  Value iterableValue = interpreter->evaluate(*node.iterable);

  // Check if it's an array
  if (auto *arrayValue = std::get_if<ArrayValue>(&iterableValue)) {
    // Create a new scope for the loop variable
    auto loopScope = std::make_shared<Environment>(interpreter->environment);

    // Iterate through array elements
    for (const auto &element : *arrayValue->elements) {
      // Set the loop variable in the new scope
      loopScope->define(node.variable, element);

      // Temporarily switch to loop scope
      auto oldEnv = interpreter->environment;
      interpreter->environment = loopScope;

      try {
        // Execute the loop body
        interpreter->execute(*node.body);
      } catch (...) {
        // Restore environment on exception
        interpreter->environment = oldEnv;
        throw;
      }

      // Restore environment
      interpreter->environment = oldEnv;
    }
  } else {
    throw std::runtime_error("Can only iterate over arrays");
  }

  result = Value();
}

void EvalVisitor::visit(WhenStmt &node) {
  // Evaluate the subject expression
  Value subjectValue = interpreter->evaluate(*node.subject);

  // Check each branch
  for (const auto &branch : node.branches) {
    Value conditionValue = interpreter->evaluate(*branch.first);

    // Check if condition matches subject
    bool matches = false;
    if (auto *subjectInt = std::get_if<int>(&subjectValue)) {
      if (auto *conditionInt = std::get_if<int>(&conditionValue)) {
        matches = (*subjectInt == *conditionInt);
      }
    } else if (auto *subjectStr = std::get_if<std::string>(&subjectValue)) {
      if (auto *conditionStr = std::get_if<std::string>(&conditionValue)) {
        matches = (*subjectStr == *conditionStr);
      }
    }

    if (matches) {
      // Execute the matching branch
      interpreter->execute(*branch.second);
      result = Value();
      return;
    }
  }

  // Check for else branch
  if (node.elseBranch) {
    interpreter->execute(**node.elseBranch);
  }

  result = Value();
}

void EvalVisitor::visit(TryStmt &node) {
  (void)node;
  result = Value();
}

void EvalVisitor::visit(ConstructorDeclStmt &node) {
  (void)node;
  result = Value();
}

void EvalVisitor::visit(WhileStmt &node) {
  (void)node;
  result = Value();
}
