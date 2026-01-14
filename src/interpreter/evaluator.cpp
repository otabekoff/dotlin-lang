#include "dotlin/parser.h"
#include "dotlin/visitors.h"
// #include <algorithm>
// #include <cmath>
#include <iostream>
// #include <sstream>
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

void EvalVisitor::visit(IdentifierExpr &node) {
  // Check if this is a special identifier like "args"
  if (node.name == "args") {
    // Return the command-line arguments array
    ArrayValue argsArray;
    for (const auto &arg : interpreter->commandLineArgs) {
      argsArray.elements.push_back(Value(arg));
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
      result = Value(std::string("undefined"));
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
    if (auto *leftStr = std::get_if<std::string>(&left)) {
      if (auto *rightStr = std::get_if<std::string>(&right)) {
        result = Value(*leftStr + *rightStr);
        return;
      }
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
  // Evaluate the callee (function or method)
  Value calleeValue = interpreter->evaluate(*node.callee);

  // Check if this is a method call on a class instance
  if (auto *memberAccess =
          dynamic_cast<MemberAccessExpr *>(node.callee.get())) {
    // Handle method calls like obj.method()
    auto objValue = interpreter->evaluate(*memberAccess->object);
    std::string methodName = memberAccess->property;

    if (auto *instance =
            std::get_if<std::shared_ptr<ClassInstance>>(&objValue)) {
      // Look up the method in the class definition
      for (const auto &method : (*instance)->classDef->methods) {
        if (method->name == methodName) {
          // Found the method, execute it
          // Create new environment for method execution with 'this'
          // bound to the instance
          auto methodEnv =
              std::make_shared<Environment>(interpreter->environment);

          // Bind 'this' to the instance
          methodEnv->define("this", Value(*instance));

          // Bind method parameters to arguments
          for (size_t i = 0; i < method->parameters.size(); ++i) {
            if (i < node.arguments.size()) {
              Value argValue = interpreter->evaluate(*node.arguments[i]);
              methodEnv->define(method->parameters[i].name, argValue);
            } else {
              // Default parameter value
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
    throw std::runtime_error("Cannot call method on non-object");
  }

  // Handle regular function calls
  if (auto *lambda = std::get_if<std::shared_ptr<LambdaValue>>(&calleeValue)) {
    // Check if this is a built-in function
    if ((*lambda)->parameters.empty() && (*lambda)->body == nullptr) {
      // This is a built-in function
      // Convert unique_ptr to shared_ptr for built-in function call
      std::vector<std::shared_ptr<Expression>> sharedArgs;
      for (const auto &arg : node.arguments) {
        sharedArgs.push_back(
            std::shared_ptr<Expression>(arg.get(), [](Expression *) {}));
      }
      result = interpreter->executeBuiltinFunction("", sharedArgs);
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

  // Check if the object is an identifier "args" and the property is "size" or
  // "contentToString"
  if (auto *identifier = dynamic_cast<IdentifierExpr *>(node.object.get())) {
    if (identifier->name == "args") {
      if (node.property == "size") {
        if (auto *argsArray = std::get_if<ArrayValue>(&objValue)) {
          result = Value(static_cast<int>(argsArray->elements.size()));
          return;
        }
      } else if (node.property == "contentToString") {
        if (auto *argsArray = std::get_if<ArrayValue>(&objValue)) {
          std::string content = "[";
          for (size_t i = 0; i < argsArray->elements.size(); ++i) {
            if (i > 0) {
              content += ", ";
            }
            content += valueToString(argsArray->elements[i]);
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

    // Look up the property in the class definition (for methods)
    for (const auto &method : (*instance)->classDef->methods) {
      if (method->name == node.property) {
        // Return a lambda that represents the method
        auto methodLambda = std::make_shared<LambdaValue>(
            method->parameters, method->body, interpreter->environment);
        result = Value(methodLambda);
        return;
      }
    }

    throw std::runtime_error("Property '" + node.property + "' not found");
  }

  // Check if the object is an array
  if (auto *array = std::get_if<ArrayValue>(&objValue)) {
    if (node.property == "size") {
      result = Value(static_cast<int>(array->elements.size()));
      return;
    }
    if (node.property == "contentToString") {
      std::string content = "[";
      for (size_t i = 0; i < array->elements.size(); ++i) {
        if (i > 0) {
          content += ", ";
        }
        content += valueToString(array->elements[i]);
      }
      content += "]";
      result = Value(content);
      return;
    }
  }

  throw std::runtime_error("Invalid member access");
}

void EvalVisitor::visit(ArrayLiteralExpr &node) {
  ArrayValue array;
  for (const auto &element : node.elements) {
    array.elements.push_back(interpreter->evaluate(*element));
  }
  result = Value(array);
}

void EvalVisitor::visit(ArrayAccessExpr &node) {
  Value arrayValue = interpreter->evaluate(*node.array);
  Value indexValue = interpreter->evaluate(*node.index);

  if (auto *array = std::get_if<ArrayValue>(&arrayValue)) {
    if (auto *index = std::get_if<int>(&indexValue)) {
      if (*index >= 0 && *index < static_cast<int>(array->elements.size())) {
        result = array->elements[static_cast<size_t>(*index)];
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
  (void)node;
  result = Value();
}

void EvalVisitor::visit(WhenStmt &node) {
  (void)node;
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
