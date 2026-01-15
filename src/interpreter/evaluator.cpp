#include "dotlin/interpreter.h"
#include "dotlin/parser.h"
#include "dotlin/visitors.h"
#include <algorithm>
// #include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>

namespace dotlin
{
  class Interpreter;
  std::string getTypeOfValue(const Value &value);
  std::string typeToString(const std::shared_ptr<Type> &type);
  std::string valueToString(const Value &value);
  bool valuesEqual(const Value &v1, const Value &v2);
} // namespace dotlin

using namespace dotlin;

// Expression evaluation visitor implementations
void EvalVisitor::visit(LiteralExpr &node)
{
  std::visit([this](auto &&arg)
             { result = Value(arg); }, node.value);
}

void EvalVisitor::visit(StringInterpolationExpr &node)
{
  std::string resultStr = "";
  for (const auto &part : node.parts)
  {
    Value partValue = interpreter->evaluate(*part);
    resultStr += interpreter->valueToString(partValue);
  }
  result = Value(resultStr);
}

void EvalVisitor::visit(IdentifierExpr &node)
{
  // Check if this is a special identifier like "args"
  if (node.name == "args")
  {
    // Return the command-line arguments array
    ArrayValue argsArray;
    for (const auto &arg : interpreter->commandLineArgs)
    {
      argsArray.elements->push_back(Value(arg));
    }
    result = Value(argsArray);
    return;
  }
  // Special handling for "this" keyword
  else if (node.name == "this")
  {
    try
    {
      result = interpreter->environment->get("this");
    }
    catch (const std::exception &e)
    {
      result = Value(std::string("undefined"));
    }
  }
  // Look up the value in the environment
  else
  {
    try
    {
      result = interpreter->environment->get(node.name);
    }
    catch (const std::runtime_error &e)
    {
      // Try to look up in 'this' context if available
      try
      {
        Value thisVal = interpreter->environment->get("this");
        if (auto *instance =
                std::get_if<std::shared_ptr<ClassInstance>>(&thisVal))
        {
          auto it = (*instance)->fields.find(node.name);
          if (it != (*instance)->fields.end())
          {
            result = it->second;
            return;
          }
        }
      }
      catch (...)
      {
        // 'this' not defined or not an instance, ignore
      }

      // Check if this is a built-in function
      if (node.name == "println" || node.name == "print" ||
          node.name == "sqrt" || node.name == "abs" || node.name == "pow" ||
          node.name == "readln" || node.name == "arrayOf" ||
          node.name == "sin" || node.name == "cos" || node.name == "tan" ||
          node.name == "min" || node.name == "max" || node.name == "round" ||
          node.name == "ceil" || node.name == "floor" ||
          node.name == "random" || node.name == "clock" ||
          node.name == "exit" || node.name == "readLine" ||
          node.name == "toInt" || node.name == "toString" ||
          node.name == "format" || node.name == "readFile" ||
          node.name == "writeFile" || node.name == "exists" ||
          node.name == "now" || node.name == "currentTimeMillis" ||
          node.name == "sleep" || node.name == "printStackTrace")
      {
        // Return a special lambda that represents a built-in function
        auto builtinLambda =
            std::make_shared<LambdaValue>(std::vector<FunctionParameter>(),
                                          nullptr, interpreter->environment);
        result = Value(builtinLambda);
      }
      else
      {
        throw DotlinError("Runtime", "Undefined variable: " + node.name,
                          node.line, node.column);
      }
    }
  }
}

void EvalVisitor::visit(LambdaExpr &node)
{
  // Create a lambda value
  auto lambda = std::make_shared<LambdaValue>(node.parameters, node.body,
                                              interpreter->environment, &node);
  result = Value(lambda);
}

void EvalVisitor::visit(BinaryExpr &node)
{
  if (node.op == TokenType::ASSIGN)
  {
    // Handle assignment
    if (auto *ident = dynamic_cast<IdentifierExpr *>(node.left.get()))
    {
      Value value = interpreter->evaluate(*node.right);
      interpreter->environment->assign(ident->name, value);
      result = value;
      return;
    }
    // Handle member access assignment (e.g., this.name = value)
    else if (auto *memberAccess =
                 dynamic_cast<MemberAccessExpr *>(node.left.get()))
    {
      Value value = interpreter->evaluate(*node.right);
      Value objValue = interpreter->evaluate(*memberAccess->object);

      if (auto *instance =
              std::get_if<std::shared_ptr<ClassInstance>>(&objValue))
      {
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

  auto handleArithmetic = [&](TokenType opType, auto op) -> bool
  {
    if (node.op == opType)
    {
      if (std::holds_alternative<double>(left) ||
          std::holds_alternative<double>(right))
      {
        double l = std::holds_alternative<double>(left)
                       ? std::get<double>(left)
                       : (std::holds_alternative<int64_t>(left)
                              ? static_cast<double>(std::get<int64_t>(left))
                              : static_cast<double>(std::get<int>(left)));
        double r = std::holds_alternative<double>(right)
                       ? std::get<double>(right)
                       : (std::holds_alternative<int64_t>(right)
                              ? static_cast<double>(std::get<int64_t>(right))
                              : static_cast<double>(std::get<int>(right)));
        result = Value(op(l, r));
        return true;
      }
      else if (std::holds_alternative<int64_t>(left) ||
               std::holds_alternative<int64_t>(right))
      {
        int64_t l = std::holds_alternative<int64_t>(left)
                        ? std::get<int64_t>(left)
                        : static_cast<int64_t>(std::get<int>(left));
        int64_t r = std::holds_alternative<int64_t>(right)
                        ? std::get<int64_t>(right)
                        : static_cast<int64_t>(std::get<int>(right));
        result = Value(op(l, r));
        return true;
      }
      else
      {
        result = Value(op(std::get<int>(left), std::get<int>(right)));
        return true;
      }
    }
    return false;
  };

  if (handleArithmetic(TokenType::MINUS, std::minus<>()))
    return;
  if (handleArithmetic(TokenType::MULTIPLY, std::multiplies<>()))
    return;

  if (node.op == TokenType::PLUS)
  {
    if (std::holds_alternative<double>(left) ||
        std::holds_alternative<double>(right))
    {
      double l = std::holds_alternative<double>(left)
                     ? std::get<double>(left)
                     : (std::holds_alternative<int64_t>(left)
                            ? static_cast<double>(std::get<int64_t>(left))
                            : static_cast<double>(std::get<int>(left)));
      double r = std::holds_alternative<double>(right)
                     ? std::get<double>(right)
                     : (std::holds_alternative<int64_t>(right)
                            ? static_cast<double>(std::get<int64_t>(right))
                            : static_cast<double>(std::get<int>(right)));
      result = Value(l + r);
      return;
    }
    else if (std::holds_alternative<int64_t>(left) ||
             std::holds_alternative<int64_t>(right))
    {
      int64_t l = std::holds_alternative<int64_t>(left)
                      ? std::get<int64_t>(left)
                      : static_cast<int64_t>(std::get<int>(left));
      int64_t r = std::holds_alternative<int64_t>(right)
                      ? std::get<int64_t>(right)
                      : static_cast<int64_t>(std::get<int>(right));
      result = Value(l + r);
      return;
    }
    else if (std::holds_alternative<int>(left) &&
             std::holds_alternative<int>(right))
    {
      result = Value(std::get<int>(left) + std::get<int>(right));
      return;
    }

    // Handle string concatenation
    if (std::holds_alternative<std::string>(left) ||
        std::holds_alternative<std::string>(right))
    {
      result = Value(interpreter->valueToString(left) +
                     interpreter->valueToString(right));
      return;
    }
    throw DotlinError(
        "Runtime",
        "Invalid operands for + operator: " + getTypeOfValue(left) + " and " +
            getTypeOfValue(right),
        node.line, node.column);
  }

  if (node.op == TokenType::DIVIDE)
  {
    auto isZero = [](const Value &v)
    {
      if (auto *i = std::get_if<int>(&v))
        return *i == 0;
      if (auto *d = std::get_if<double>(&v))
        return *d == 0.0;
      return false;
    };
    if (isZero(right))
    {
      throw DotlinError("Runtime", "Division by zero", node.line, node.column);
    }

    if (std::holds_alternative<double>(left) ||
        std::holds_alternative<double>(right))
    {
      double l = std::holds_alternative<double>(left)
                     ? std::get<double>(left)
                     : (std::holds_alternative<int64_t>(left)
                            ? static_cast<double>(std::get<int64_t>(left))
                            : static_cast<double>(std::get<int>(left)));
      double r = std::holds_alternative<double>(right)
                     ? std::get<double>(right)
                     : (std::holds_alternative<int64_t>(right)
                            ? static_cast<double>(std::get<int64_t>(right))
                            : static_cast<double>(std::get<int>(right)));
      result = Value(l / r);
      return;
    }
    else if (std::holds_alternative<int64_t>(left) ||
             std::holds_alternative<int64_t>(right))
    {
      int64_t l = std::holds_alternative<int64_t>(left)
                      ? std::get<int64_t>(left)
                      : static_cast<int64_t>(std::get<int>(left));
      int64_t r = std::holds_alternative<int64_t>(right)
                      ? std::get<int64_t>(right)
                      : static_cast<int64_t>(std::get<int>(right));
      result = Value(l / r);
      return;
    }
    else
    {
      result = Value(std::get<int>(left) / std::get<int>(right));
      return;
    }
    throw DotlinError(
        "Runtime",
        "Invalid operands for / operator: " + getTypeOfValue(left) + " and " +
            getTypeOfValue(right),
        node.line, node.column);
  }

  if (node.op == TokenType::MODULO)
  {
    if (std::holds_alternative<int64_t>(left) ||
        std::holds_alternative<int64_t>(right))
    {
      int64_t l = std::holds_alternative<int64_t>(left)
                      ? std::get<int64_t>(left)
                      : static_cast<int64_t>(std::get<int>(left));
      int64_t r = std::holds_alternative<int64_t>(right)
                      ? std::get<int64_t>(right)
                      : static_cast<int64_t>(std::get<int>(right));
      if (r == 0)
        throw DotlinError("Runtime", "Modulo by zero", node.line, node.column);
      result = Value(l % r);
      return;
    }
    else if (auto *lInt = std::get_if<int>(&left))
    {
      if (auto *rInt = std::get_if<int>(&right))
      {
        if (*rInt == 0)
          throw DotlinError("Runtime", "Modulo by zero", node.line,
                            node.column);
        result = Value(*lInt % *rInt);
        return;
      }
    }
    throw DotlinError(
        "Runtime",
        "Invalid operands for % operator: " + getTypeOfValue(left) + " and " +
            getTypeOfValue(right),
        node.line, node.column);
  }

  // Handle comparison operations
  auto handleComparison = [&](TokenType opType) -> bool
  {
    if (node.op == opType)
    {
      double lVal, rVal;
      bool hasL = false, hasR = false;

      if (auto *lInt = std::get_if<int>(&left))
      {
        lVal = static_cast<double>(*lInt);
        hasL = true;
      }
      else if (auto *lLong = std::get_if<int64_t>(&left))
      {
        lVal = static_cast<double>(*lLong);
        hasL = true;
      }
      else if (auto *lDouble = std::get_if<double>(&left))
      {
        lVal = *lDouble;
        hasL = true;
      }

      if (auto *rInt = std::get_if<int>(&right))
      {
        rVal = static_cast<double>(*rInt);
        hasR = true;
      }
      else if (auto *rLong = std::get_if<int64_t>(&right))
      {
        rVal = static_cast<double>(*rLong);
        hasR = true;
      }
      else if (auto *rDouble = std::get_if<double>(&right))
      {
        rVal = *rDouble;
        hasR = true;
      }

      if (hasL && hasR)
      {
        if (opType == TokenType::LESS)
          result = Value(lVal < rVal);
        else if (opType == TokenType::LESS_EQUAL)
          result = Value(lVal <= rVal);
        else if (opType == TokenType::GREATER)
          result = Value(lVal > rVal);
        else if (opType == TokenType::GREATER_EQUAL)
          result = Value(lVal >= rVal);
        return true;
      }
    }
    return false;
  };

  if (handleComparison(TokenType::LESS))
    return;
  if (handleComparison(TokenType::LESS_EQUAL))
    return;
  if (handleComparison(TokenType::GREATER))
    return;
  if (handleComparison(TokenType::GREATER_EQUAL))
    return;

  if (node.op == TokenType::EQUAL)
  {
    result = Value(dotlin::valuesEqual(left, right));
    return;
  }

  if (node.op == TokenType::NOT_EQUAL)
  {
    result = Value(!dotlin::valuesEqual(left, right));
    return;
  }

  throw std::runtime_error("Unknown binary operator");
}

void EvalVisitor::visit(UnaryExpr &node)
{
  Value operand = interpreter->evaluate(*node.operand);

  if (node.op == TokenType::MINUS)
  {
    if (auto *intValue = std::get_if<int>(&operand))
    {
      result = Value(-*intValue);
      return;
    }
    throw DotlinError("Runtime",
                      "Invalid operand for unary minus: " +
                          getTypeOfValue(operand),
                      node.line, node.column);
  }

  if (node.op == TokenType::NOT)
  {
    if (auto *boolValue = std::get_if<bool>(&operand))
    {
      result = Value(!*boolValue);
      return;
    }
    throw DotlinError("Runtime",
                      "Invalid operand for logical not: " +
                          getTypeOfValue(operand),
                      node.line, node.column);
  }

  throw std::runtime_error("Unknown unary operator");
}

void EvalVisitor::visit(CallExpr &node)
{
  if (!node.callee)
  {
    throw std::runtime_error("Internal Error: AST node.callee is null");
  }
  // Check if this is a method call on a class instance or built-in type
  if (auto *memberAccess =
          dynamic_cast<MemberAccessExpr *>(node.callee.get()))
  {
    if (!memberAccess->object)
    {
      throw std::runtime_error("Internal Error: AST corruption");
    }
    // Handle method calls like obj.method()
    auto objValue = interpreter->evaluate(*memberAccess->object);
    std::string methodName = memberAccess->property;

    // Evaluate arguments first (needed for string methods)
    std::vector<Value> args;
    for (const auto &arg : node.arguments)
    {
      if (arg)
      {
        args.push_back(interpreter->evaluate(*arg));
      }
    }

    // Handle method calls on built-in types (strings, arrays, etc.)
    if (methodName == "toString" && args.empty())
    {
      result = Value(interpreter->valueToString(objValue));
      return;
    }
    else if (auto *array = std::get_if<ArrayValue>(&objValue))
    {
      if (methodName == "size" && args.empty())
      {
        result = Value(static_cast<int>(array->elements->size()));
        return;
      }
      else if (methodName == "contentToString" && args.empty())
      {
        std::string content = "[";
        for (size_t i = 0; i < array->elements->size(); ++i)
        {
          if (i > 0)
          {
            content += ", ";
          }
          content += interpreter->valueToString((*array->elements)[i]);
        }
        content += "]";
        result = Value(content);
        return;
      }
      else if (methodName == "add")
      {
        if (args.size() == 1)
        {
          array->push_back(args[0]);
          result = Value(true);
          return;
        }
        else
        {
          throw std::runtime_error("add method requires 1 argument");
        }
      }
      else if (methodName == "get")
      {
        if (args.size() == 1 && std::holds_alternative<int>(args[0]))
        {
          int index = std::get<int>(args[0]);
          result = array->get(static_cast<size_t>(index));
          return;
        }
        else
        {
          throw std::runtime_error("get method requires 1 integer argument");
        }
      }
      else if (methodName == "set")
      {
        if (args.size() == 2 && std::holds_alternative<int>(args[0]))
        {
          int index = std::get<int>(args[0]);
          array->set(static_cast<size_t>(index), args[1]);
          result = args[1]; // Return the assigned value
          return;
        }
        else
        {
          throw std::runtime_error("set method requires index and value");
        }
      }
      else if (methodName == "removeAt")
      {
        if (args.size() == 1 && std::holds_alternative<int>(args[0]))
        {
          int index = std::get<int>(args[0]);
          array->removeAt(static_cast<size_t>(index));
          result = Value(true);
          return;
        }
        else
        {
          throw std::runtime_error(
              "removeAt method requires 1 integer argument");
        }
      }
      else if (methodName == "insert")
      {
        if (args.size() == 2 && std::holds_alternative<int>(args[0]))
        {
          int index = std::get<int>(args[0]);
          array->insert(static_cast<size_t>(index), args[1]);
          result = Value(true);
          return;
        }
        else
        {
          throw std::runtime_error("insert method requires (index, value)");
        }
      }
      else if (methodName == "remove")
      {
        if (args.size() == 1)
        {
          Value elementToRemove = args[0];
          bool found = false;
          for (size_t i = 0; i < array->elements->size(); ++i)
          {
            if (valuesEqual((*array->elements)[i], elementToRemove))
            {
              array->removeAt(i);
              found = true;
              break;
            }
          }
          result = Value(found);
          return;
        }
        else
        {
          throw std::runtime_error("remove method requires 1 argument");
        }
      }
      else if (methodName == "indexOf")
      {
        if (args.size() == 1)
        {
          Value elementToFind = args[0];
          int foundIndex = -1;
          for (size_t i = 0; i < array->elements->size(); ++i)
          {
            if (valuesEqual((*array->elements)[i], elementToFind))
            {
              foundIndex = static_cast<int>(i);
              break;
            }
          }
          result = Value(foundIndex);
          return;
        }
        else
        {
          throw std::runtime_error("indexOf method requires 1 argument");
        }
      }
      else if (methodName == "contains")
      {
        if (args.size() == 1)
        {
          Value elementToFind = args[0];
          bool found = false;
          for (const auto &element : *array->elements)
          {
            if (valuesEqual(element, elementToFind))
            {
              found = true;
              break;
            }
          }
          result = Value(found);
          return;
        }
        else
        {
          throw std::runtime_error("contains method requires 1 argument");
        }
      }
      else if (methodName == "clear")
      {
        if (args.empty())
        {
          array->elements->clear();
          result = Value(); // Unit/Void
          return;
        }
        else
        {
          throw std::runtime_error("clear method takes no arguments");
        }
      }
      else if (methodName == "isEmpty")
      {
        if (args.empty())
        {
          result = Value(array->elements->empty());
          return;
        }
        else
        {
          throw std::runtime_error("isEmpty method takes no arguments");
        }
      }
      else if (methodName == "map")
      {
        if (args.size() == 1)
        {
          Value callback = args[0];
          if (auto *lambda =
                  std::get_if<std::shared_ptr<LambdaValue>>(&callback))
          {
            ArrayValue resultArr;
            for (const auto &element : *array->elements)
            {
              auto closure = (*lambda)->closure;
              auto lambdaEnv = std::make_shared<Environment>(closure);
              if (!(*lambda)->parameters.empty())
              {
                lambdaEnv->define((*lambda)->parameters[0].name, element);
              }
              else
              {
                lambdaEnv->define("it", element);
              }
              auto prevEnv = interpreter->environment;
              auto prevFuncEnv = interpreter->functionEnvironment;
              interpreter->environment = lambdaEnv;
              interpreter->functionEnvironment = lambdaEnv;
              Value mappedVal;
              try
              {
                mappedVal = interpreter->executeFunction(
                    "lambda@map", (*lambda)->body.get(), lambdaEnv);
              }
              catch (DotlinError &e)
              {
                if (e.stackTrace.empty())
                  e.setStackTrace(interpreter->callStack);
                interpreter->environment = prevEnv;
                interpreter->functionEnvironment = prevFuncEnv;
                throw;
              }
              catch (...)
              {
                interpreter->environment = prevEnv;
                interpreter->functionEnvironment = prevFuncEnv;
                throw;
              }
              interpreter->environment = prevEnv;
              interpreter->functionEnvironment = prevFuncEnv;
              resultArr.elements->push_back(mappedVal);
            }
            result = Value(resultArr);
            return;
          }
          throw std::runtime_error("map expects a lambda function");
        }
        throw std::runtime_error("map expects 1 argument");
      }
      else if (methodName == "filter")
      {
        if (args.size() == 1)
        {
          Value callback = args[0];
          if (auto *lambda =
                  std::get_if<std::shared_ptr<LambdaValue>>(&callback))
          {
            ArrayValue resultArr;
            for (const auto &element : *array->elements)
            {
              auto closure = (*lambda)->closure;
              auto lambdaEnv = std::make_shared<Environment>(closure);
              if (!(*lambda)->parameters.empty())
              {
                lambdaEnv->define((*lambda)->parameters[0].name, element);
              }
              else
              {
                lambdaEnv->define("it", element);
              }
              auto prevEnv = interpreter->environment;
              auto prevFuncEnv = interpreter->functionEnvironment;
              interpreter->environment = lambdaEnv;
              interpreter->functionEnvironment = lambdaEnv;
              Value funcResult;
              try
              {
                funcResult = interpreter->executeFunction(
                    "lambda@filter", (*lambda)->body.get(), lambdaEnv);
              }
              catch (DotlinError &e)
              {
                if (e.stackTrace.empty())
                  e.setStackTrace(interpreter->callStack);
                interpreter->environment = prevEnv;
                interpreter->functionEnvironment = prevFuncEnv;
                throw;
              }
              catch (...)
              {
                interpreter->environment = prevEnv;
                interpreter->functionEnvironment = prevFuncEnv;
                throw;
              }
              interpreter->environment = prevEnv;
              interpreter->functionEnvironment = prevFuncEnv;

              bool keep = false;
              if (auto *b = std::get_if<bool>(&funcResult))
              {
                keep = *b;
              }
              if (keep)
              {
                resultArr.elements->push_back(element);
              }
            }
            result = Value(resultArr);
            return;
          }
          throw std::runtime_error("filter expects a lambda function");
        }
        throw std::runtime_error("filter expects 1 argument");
      }
    }
    else if (methodName == "substring" && args.size() >= 1)
    {
      // Handle substring method calls
      if (auto *strValue = std::get_if<std::string>(&objValue))
      {
        if (args.size() == 1 && std::holds_alternative<int>(args[0]))
        {
          std::string str = *strValue;
          int start = std::get<int>(args[0]);
          if (start >= 0 && static_cast<size_t>(start) <= str.length())
          {
            result = Value(str.substr(static_cast<size_t>(start)));
          }
          else
          {
            result = Value(std::string(""));
          }
        }
        else if (args.size() == 2 && std::holds_alternative<int>(args[0]) &&
                 std::holds_alternative<int>(args[1]))
        {
          std::string str = *strValue;
          int start = std::get<int>(args[0]);
          int end = std::get<int>(args[1]);
          if (start >= 0 && static_cast<size_t>(end) <= str.length() &&
              start <= end)
          {
            result = Value(str.substr(static_cast<size_t>(start),
                                      static_cast<size_t>(end - start)));
          }
          else
          {
            result = Value(std::string(""));
          }
        }
      }
      return;
    }
    else if (methodName == "indexOf" && args.size() == 1 &&
             std::holds_alternative<std::string>(args[0]))
    {
      // Handle indexOf method calls
      if (auto *strValue = std::get_if<std::string>(&objValue))
      {
        std::string str = *strValue;
        std::string substr = std::get<std::string>(args[0]);
        size_t pos = str.find(substr);
        result = Value(static_cast<int>(
            pos != std::string::npos ? static_cast<int>(pos) : -1));
      }
      else
      {
        result = Value(-1);
      }
      return;
    }
    else if (methodName == "startsWith" && args.size() == 1 &&
             std::holds_alternative<std::string>(args[0]))
    {
      // Handle startsWith method calls
      if (auto *strValue = std::get_if<std::string>(&objValue))
      {
        std::string str = *strValue;
        std::string prefix = std::get<std::string>(args[0]);
        result =
            Value(static_cast<bool>(str.substr(0, prefix.length()) == prefix));
      }
      else
      {
        result = Value(false);
      }
      return;
    }
    else if (methodName == "endsWith" && args.size() == 1 &&
             std::holds_alternative<std::string>(args[0]))
    {
      // Handle endsWith method calls
      if (auto *strValue = std::get_if<std::string>(&objValue))
      {
        std::string str = *strValue;
        std::string suffix = std::get<std::string>(args[0]);
        if (str.length() >= suffix.length())
        {
          result =
              Value(static_cast<bool>(str.substr(str.length() - suffix.length(),
                                                 suffix.length()) == suffix));
        }
        else
        {
          result = Value(false);
        }
      }
      else
      {
        result = Value(false);
      }
      return;
    }
    else if (methodName == "toUpperCase" && args.empty())
    {
      // Handle toUpperCase method calls
      if (auto *strValue = std::get_if<std::string>(&objValue))
      {
        std::string str = *strValue;
        std::transform(str.begin(), str.end(), str.begin(), ::toupper);
        result = Value(str);
      }
      else
      {
        result = Value(std::string(""));
      }
      return;
    }
    else if (methodName == "toLowerCase" && args.empty())
    {
      // Handle toLowerCase method calls
      if (auto *strValue = std::get_if<std::string>(&objValue))
      {
        std::string str = *strValue;
        std::transform(str.begin(), str.end(), str.begin(), ::tolower);
        result = Value(str);
      }
      else
      {
        result = Value(std::string(""));
      }
      return;
    }
    else if (methodName == "trim" && args.empty())
    {
      // Handle trim method calls
      if (auto *strValue = std::get_if<std::string>(&objValue))
      {
        std::string str = *strValue;
        size_t start = str.find_first_not_of(" \t\n\r\f\v");
        if (start == std::string::npos)
        {
          result = Value(str);
        }
        else
        {
          size_t end = str.find_last_not_of(" \t\n\r\f\v");
          result = Value(str.substr(start, end - start + 1));
        }
      }
      else
      {
        result = Value(std::string(""));
      }
      return;
    }
    else if (methodName == "split" && args.size() == 1 &&
             std::holds_alternative<std::string>(args[0]))
    {
      // Handle split method calls
      if (auto *strValue = std::get_if<std::string>(&objValue))
      {
        std::string str = *strValue;
        std::string delim = std::get<std::string>(args[0]);
        std::vector<Value> parts;
        size_t pos = 0;
        size_t delimLen = delim.length();
        std::string remaining = str;

        while ((pos = remaining.find(delim, pos)) != std::string::npos)
        {
          parts.push_back(Value(remaining.substr(0, pos)));
          pos += delimLen;
          remaining = remaining.substr(pos);
          pos = 0;
        }
        // Add the remaining part
        parts.push_back(Value(remaining));
        result = Value(ArrayValue(parts));
      }
      else
      {
        result = Value(ArrayValue());
      }
      return;
    }
    else if (methodName == "toInt" && args.empty())
    {
      // Handle toInt method calls on String
      if (auto *strValue = std::get_if<std::string>(&objValue))
      {
        try
        {
          result = Value(std::stoi(*strValue));
        }
        catch (...)
        {
          throw std::runtime_error("Invalid number format for toInt: " +
                                   *strValue);
        }
      }
      else if (auto *doubleValue = std::get_if<double>(&objValue))
      {
        // Allow double -> int conversion via toInt()
        result = Value(static_cast<int>(*doubleValue));
      }
      else if (auto *longValue = std::get_if<int64_t>(&objValue))
      {
        // Identity/downcast conversion
        result = Value(static_cast<int>(*longValue));
      }
      else
      {
        throw std::runtime_error(
            "toInt method only supported on String, Double, Int, and Long");
      }
      return;
    }
    else if (methodName == "toDouble" && args.empty())
    {
      // Handle toDouble method calls on String
      if (auto *strValue = std::get_if<std::string>(&objValue))
      {
        try
        {
          result = Value(std::stod(*strValue));
        }
        catch (...)
        {
          throw std::runtime_error("Invalid number format for toDouble: " +
                                   *strValue);
        }
      }
      else if (auto *longValue = std::get_if<int64_t>(&objValue))
      {
        // Allow long -> double conversion via toDouble()
        result = Value(static_cast<double>(*longValue));
      }
      else
      {
        throw std::runtime_error(
            "toDouble method only supported on String, Int, and Long");
      }
      return;
    }

    // Check if the object is a class instance
    if (auto *instance =
            std::get_if<std::shared_ptr<ClassInstance>>(&objValue))
    {
      // Look up the method in the class definition or superclasses
      std::shared_ptr<ClassDefinition> currentClass = (*instance)->classDef;

      while (currentClass)
      {
        for (const auto &method : currentClass->methods)
        {
          if (method->name == methodName)
          {
            // Found the method, execute it
            auto methodEnv =
                std::make_shared<Environment>(interpreter->environment);
            methodEnv->define("this", Value(*instance));

            // Bind method parameters to arguments
            for (size_t i = 0; i < method->parameters.size(); ++i)
            {
              if (i < node.arguments.size())
              {
                Value argValue = interpreter->evaluate(*node.arguments[i]);
                methodEnv->define(method->parameters[i].name, argValue);
              }
              else
              {
                methodEnv->define(method->parameters[i].name, Value());
              }
            }

            // Switch to method environment
            interpreter->environment = methodEnv;
            interpreter->functionEnvironment = methodEnv;

            // Execute method body
            Value returnValue;
            try
            {
              returnValue = interpreter->executeFunction(
                  (*instance)->className + "." + methodName, method->body.get(),
                  methodEnv);
            }
            catch (DotlinError &e)
            {
              if (e.stackTrace.empty())
                e.setStackTrace(interpreter->callStack);
              interpreter->environment = methodEnv->enclosing;
              interpreter->functionEnvironment = methodEnv->enclosing;
              throw;
            }
            catch (...)
            {
              interpreter->environment = methodEnv->enclosing;
              interpreter->functionEnvironment = methodEnv->enclosing;
              throw;
            }
            interpreter->environment = methodEnv->enclosing;
            interpreter->functionEnvironment = methodEnv->enclosing;

            result = returnValue;
            return;
          }
        }
        // Move to superclass
        currentClass = currentClass->superclass;
      }

      // Method not found in class hierarchy, check for extension functions
      std::string objTypeName = (*instance)->className;
      std::string extensionFuncName = "ext_" + objTypeName + "_" + methodName;

      try
      {
        Value extFuncValue = interpreter->environment->get(extensionFuncName);
        if (auto *lambda = std::get_if<std::shared_ptr<LambdaValue>>(&extFuncValue))
        {
          // Create new environment for the extension function
          auto extFuncEnv = std::make_shared<Environment>((*lambda)->closure);

          // The first parameter of an extension function is implicitly the receiver
          if (!(*lambda)->parameters.empty())
          {
            // Add the object instance as the first argument
            extFuncEnv->define((*lambda)->parameters[0].name, Value(*instance));

            // Bind the rest of the parameters to the arguments
            for (size_t i = 1; i < (*lambda)->parameters.size(); ++i)
            {
              if (i - 1 < node.arguments.size())
              {
                Value argValue = interpreter->evaluate(*node.arguments[i - 1]);
                extFuncEnv->define((*lambda)->parameters[i].name, argValue);
              }
              else
              {
                extFuncEnv->define((*lambda)->parameters[i].name, Value());
              }
            }
          }
          else
          {
            // No parameters, just bind arguments if any
            for (size_t i = 0; i < node.arguments.size(); ++i)
            {
              if (i < (*lambda)->parameters.size())
              {
                Value argValue = interpreter->evaluate(*node.arguments[i]);
                extFuncEnv->define((*lambda)->parameters[i].name, argValue);
              }
            }
          }

          // Switch to extension function environment
          interpreter->environment = extFuncEnv;
          interpreter->functionEnvironment = extFuncEnv;

          // Execute extension function body
          Value returnValue;
          try
          {
            returnValue = interpreter->executeFunction(
                extensionFuncName, (*lambda)->body.get(), extFuncEnv);
          }
          catch (DotlinError &e)
          {
            if (e.stackTrace.empty())
              e.setStackTrace(interpreter->callStack);
            interpreter->environment = extFuncEnv->enclosing;
            interpreter->functionEnvironment = extFuncEnv->enclosing;
            throw;
          }
          catch (...)
          {
            interpreter->environment = extFuncEnv->enclosing;
            interpreter->functionEnvironment = extFuncEnv->enclosing;
            throw;
          }
          interpreter->environment = extFuncEnv->enclosing;
          interpreter->functionEnvironment = extFuncEnv->enclosing;

          result = returnValue;
          return;
        }
      }
      catch (const std::exception &)
      {
        // Extension function not found, continue with error
      }

      throw std::runtime_error("Method '" + methodName + "' not found");
    }

    // Check for extension functions on primitive types
    std::string objTypeName = getTypeOfValue(objValue);
    // Map the lowercase type names to the capitalized type names used in extension functions
    std::string capitalizedTypeName = objTypeName;
    if (objTypeName == "int")
      capitalizedTypeName = "Int";
    else if (objTypeName == "long")
      capitalizedTypeName = "Long";
    else if (objTypeName == "double")
      capitalizedTypeName = "Double";
    else if (objTypeName == "bool")
      capitalizedTypeName = "Boolean";
    else if (objTypeName == "string")
      capitalizedTypeName = "String";
    else if (objTypeName == "Array")
      capitalizedTypeName = "Array";
    else if (objTypeName == "Object")
    {
      // For class instances, we need to get the class name
      if (auto *instance = std::get_if<std::shared_ptr<ClassInstance>>(&objValue))
      {
        capitalizedTypeName = (*instance)->className;
      }
    }

    std::string extensionFuncName = "ext_" + capitalizedTypeName + "_" + methodName;

    try
    {
      Value extFuncValue = interpreter->environment->get(extensionFuncName);
      if (auto *lambda = std::get_if<std::shared_ptr<LambdaValue>>(&extFuncValue))
      {
        // Create new environment for the extension function
        auto extFuncEnv = std::make_shared<Environment>((*lambda)->closure);

        // The first parameter of an extension function is implicitly the receiver
        if (!(*lambda)->parameters.empty())
        {
          // Add the object value as the first argument
          extFuncEnv->define((*lambda)->parameters[0].name, objValue);

          // Bind the rest of the parameters to the arguments
          for (size_t i = 1; i < (*lambda)->parameters.size(); ++i)
          {
            if (i - 1 < node.arguments.size())
            {
              Value argValue = interpreter->evaluate(*node.arguments[i - 1]);
              extFuncEnv->define((*lambda)->parameters[i].name, argValue);
            }
            else
            {
              extFuncEnv->define((*lambda)->parameters[i].name, Value());
            }
          }
        }
        else
        {
          // No parameters, just bind arguments if any
          for (size_t i = 0; i < node.arguments.size(); ++i)
          {
            if (i < (*lambda)->parameters.size())
            {
              Value argValue = interpreter->evaluate(*node.arguments[i]);
              extFuncEnv->define((*lambda)->parameters[i].name, argValue);
            }
          }
        }

        // Switch to extension function environment
        interpreter->environment = extFuncEnv;
        interpreter->functionEnvironment = extFuncEnv;

        // Execute extension function body
        Value returnValue;
        try
        {
          returnValue = interpreter->executeFunction(
              extensionFuncName, (*lambda)->body.get(), extFuncEnv);
        }
        catch (DotlinError &e)
        {
          if (e.stackTrace.empty())
            e.setStackTrace(interpreter->callStack);
          interpreter->environment = extFuncEnv->enclosing;
          interpreter->functionEnvironment = extFuncEnv->enclosing;
          throw;
        }
        catch (...)
        {
          interpreter->environment = extFuncEnv->enclosing;
          interpreter->functionEnvironment = extFuncEnv->enclosing;
          throw;
        }
        interpreter->environment = extFuncEnv->enclosing;
        interpreter->functionEnvironment = extFuncEnv->enclosing;

        result = returnValue;
        return;
      }
    }
    catch (const std::exception &)
    {
      // Extension function not found, continue with error
    }

    throw std::runtime_error("Cannot call method '" + methodName +
                             "' on this object");
  }

  // Handle regular function calls
  Value calleeValue = interpreter->evaluate(*node.callee);
  if (auto *lambda = std::get_if<std::shared_ptr<LambdaValue>>(&calleeValue))
  {
    // Check if this is a built-in function
    if ((*lambda)->parameters.empty() && (*lambda)->body == nullptr)
    {
      // This is a built-in function - get the function name
      std::string functionName = "";
      if (auto *identifier =
              dynamic_cast<IdentifierExpr *>(node.callee.get()))
      {
        functionName = identifier->name;
      }

      // Convert unique_ptr to shared_ptr for built-in function call
      std::vector<std::shared_ptr<Expression>> sharedArgs;
      for (const auto &arg : node.arguments)
      {
        sharedArgs.push_back(
            std::shared_ptr<Expression>(arg.get(), [](Expression *) {}));
      }
      result = interpreter->executeBuiltinFunction(functionName, sharedArgs);
      return;
    }

    // Create new environment for the function
    auto funcEnv = std::make_shared<Environment>((*lambda)->closure);

    // Bind parameters to arguments
    for (size_t i = 0; i < (*lambda)->parameters.size(); ++i)
    {
      if (i < node.arguments.size())
      {
        Value argValue = interpreter->evaluate(*node.arguments[i]);
        funcEnv->define((*lambda)->parameters[i].name, argValue);
      }
      else
      {
        // Default parameter value
        funcEnv->define((*lambda)->parameters[i].name, Value());
      }
    }

    // Switch to function environment
    interpreter->environment = funcEnv;
    interpreter->functionEnvironment = funcEnv;

    // Execute function body
    std::string funcName = "lambda";
    if (auto *id = dynamic_cast<IdentifierExpr *>(node.callee.get()))
    {
      funcName = id->name;
    }
    Value returnValue =
        interpreter->executeFunction(funcName, (*lambda)->body.get(), funcEnv);
    interpreter->environment = funcEnv->enclosing;
    interpreter->functionEnvironment = funcEnv->enclosing;

    result = returnValue;
  }
  else if (auto *classDef =
               std::get_if<std::shared_ptr<ClassDefinition>>(&calleeValue))
  {
    // Create a class instance
    auto instance =
        std::make_shared<ClassInstance>((*classDef)->name, *classDef);

    // Create a temporary environment to execute field initializers
    // We use the current environment as enclosing to allow access to
    // globals/etc? Actually field initializers might refer to globals.
    auto instanceEnv = std::make_shared<Environment>(interpreter->environment);

    // Collect class hierarchy directly to a vector (Base -> Derived)
    std::vector<std::shared_ptr<ClassDefinition>> hierarchy;
    std::shared_ptr<ClassDefinition> current = *classDef;
    while (current)
    {
      hierarchy.insert(hierarchy.begin(), current);
      current = current->superclass;
    }

    // Execute field declarations in order
    auto oldEnv = interpreter->environment;
    interpreter->environment = instanceEnv;

    try
    {
      for (const auto &cls : hierarchy)
      {
        for (const auto &fieldDecl : cls->fieldDecls)
        {
          if (!fieldDecl)
          {
            continue;
          }
          interpreter->execute(*fieldDecl);
        }
      }
    }
    catch (...)
    {
      interpreter->environment = oldEnv;
      throw;
    }
    interpreter->environment = oldEnv;

    // Copy initialized variables to instance fields
    instance->fields = instanceEnv->values;

    // Execute matching constructor
    if (!(*classDef)->constructors.empty())
    {
      // Evaluate all arguments once
      std::vector<Value> argValues;
      for (const auto &arg : node.arguments)
      {
        argValues.push_back(interpreter->evaluate(*arg));
      }

      // Look for a matching constructor (overload resolution)
      std::shared_ptr<FunctionDef> bestMatch = nullptr;
      for (const auto &ctor : (*classDef)->constructors)
      {
        if (ctor->parameters.size() == argValues.size())
        {
          bool typesMatch = true;
          for (size_t i = 0; i < ctor->parameters.size(); ++i)
          {
            if (ctor->parameters[i].typeAnnotation &&
                (*ctor->parameters[i].typeAnnotation)->kind !=
                    TypeKind::UNKNOWN)
            {
              std::string expected =
                  typeToString(*ctor->parameters[i].typeAnnotation);
              std::string actual = getTypeOfValue(argValues[i]);

              // Map Dotlin type names to getTypeOfValue names if needed
              // e.g., "Int" vs "int", "Long" vs "long"
              auto normalize = [](const std::string &s)
              {
                if (s == "Int")
                  return std::string("int");
                if (s == "Long")
                  return std::string("long");
                if (s == "Double")
                  return std::string("double");
                if (s == "String")
                  return std::string("string");
                if (s == "Boolean")
                  return std::string("bool");
                return s;
              };

              if (normalize(expected) != actual)
              {
                typesMatch = false;
                break;
              }
            }
          }

          if (typesMatch)
          {
            bestMatch = ctor;
            break;
          }
        }
      }

      if (bestMatch)
      {
        auto ctorEnv = std::make_shared<Environment>(interpreter->environment);
        ctorEnv->define("this", Value(instance));

        // Bind parameters
        for (size_t i = 0; i < bestMatch->parameters.size(); ++i)
        {
          ctorEnv->define(bestMatch->parameters[i].name, argValues[i]);
        }

        auto prevFuncEnv = interpreter->functionEnvironment;
        interpreter->environment = ctorEnv;
        interpreter->functionEnvironment = ctorEnv;

        try
        {
          interpreter->executeFunction((*classDef)->name + ".<init>",
                                       bestMatch->body.get(), ctorEnv);
        }
        catch (DotlinError &e)
        {
          interpreter->environment = oldEnv;
          interpreter->functionEnvironment = prevFuncEnv;
          throw;
        }
        interpreter->environment = oldEnv;
        interpreter->functionEnvironment = prevFuncEnv;
      }
      else
      {
        throw std::runtime_error("No matching constructor found for class " +
                                 (*classDef)->name + " with " +
                                 std::to_string(argValues.size()) +
                                 " arguments and matching types");
      }
    }
    else if (!node.arguments.empty())
    {

      throw std::runtime_error(
          "Class " + (*classDef)->name +
          " has no constructors but arguments were provided");
    }

    result = Value(instance);
  }
  else
  {
    throw DotlinError("Runtime", "Attempt to call a non-function value",
                      node.line, node.column);
  }
}

void EvalVisitor::visit(MemberAccessExpr &node)
{
  if (!node.object)
  {
    std::cerr << "ERROR: Member access expression has null object" << std::endl;
    return;
  }

  auto objValue = node.object ? interpreter->evaluate(*node.object)
                              : Value(std::string("null"));

  // Check if the object is an identifier "args" and the property is "size"
  // or "contentToString"
  if (auto *identifier = dynamic_cast<IdentifierExpr *>(node.object.get()))
  {
    if (identifier->name == "args")
    {
      if (node.property == "size")
      {
        if (auto *argsArray = std::get_if<ArrayValue>(&objValue))
        {
          result = Value(static_cast<int>(argsArray->elements->size()));
          return;
        }
      }
      else if (node.property == "contentToString")
      {
        if (auto *argsArray = std::get_if<ArrayValue>(&objValue))
        {
          std::string content = "[";
          for (size_t i = 0; i < argsArray->elements->size(); ++i)
          {
            if (i > 0)
            {
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
  if (auto *instance = std::get_if<std::shared_ptr<ClassInstance>>(&objValue))
  {
    // Look up the property in the instance's fields
    auto it = (*instance)->fields.find(node.property);
    if (it != (*instance)->fields.end())
    {
      result = it->second;
      return;
    }
    throw std::runtime_error("Property '" + node.property +
                             "' not found in class " + (*instance)->className);
  }

  // Check if the object is a string and has string properties/methods
  if (auto *strValue = std::get_if<std::string>(&objValue))
  {
    if (node.property == "length")
    {
      result = Value(static_cast<int>(strValue->length()));
      return;
    }
    else if (node.property == "trim")
    {
      std::string trimmedStr = *strValue;
      size_t start = trimmedStr.find_first_not_of(" \t\n\r\f\v");
      if (start == std::string::npos)
      {
        result = Value(trimmedStr);
      }
      else
      {
        size_t end = trimmedStr.find_last_not_of(" \t\n\r\f\v");
        result = Value(trimmedStr.substr(start, end - start + 1));
      }
      return;
    }
    else if (node.property == "substring" || node.property == "indexOf" ||
             node.property == "startsWith" || node.property == "endsWith" ||
             node.property == "toUpperCase" ||
             node.property == "toLowerCase" || node.property == "split")
    {
      throw std::runtime_error("Method '" + node.property +
                               "' must be called with ()");
    }
    else if (node.property == "contentToString")
    {
      std::string content = "[";
      for (size_t i = 0; i < strValue->length(); ++i)
      {
        if (i > 0)
          content += ", ";
        content += std::string(1, (*strValue)[i]); // Just the char for now
      }
      content += "]";
      result = Value(content);
      return;
    }
    throw std::runtime_error("String property '" + node.property +
                             "' not implemented");
  }

  // Check if the object is an array
  if (auto *array = std::get_if<ArrayValue>(&objValue))
  {
    if (node.property == "size")
    {
      result = Value(static_cast<int>(array->elements->size()));
      return;
    }
    if (node.property == "contentToString")
    {
      std::string content = "[";
      for (size_t i = 0; i < array->elements->size(); ++i)
      {
        if (i > 0)
          content += ", ";
        content += valueToString((*array->elements)[i]);
      }
      content += "]";
      result = Value(content);
      return;
    }

    throw std::runtime_error("Array does not have property '" + node.property +
                             "'");
  }

  throw std::runtime_error("Cannot access member '" + node.property +
                           "' on type " + getTypeOfValue(objValue));
}

void EvalVisitor::visit(ArrayLiteralExpr &node)
{
  ArrayValue array;
  for (const auto &element : node.elements)
  {
    array.elements->push_back(interpreter->evaluate(*element));
  }
  result = Value(array);
}

void EvalVisitor::visit(ArrayAccessExpr &node)
{
  Value arrayValue = interpreter->evaluate(*node.array);
  Value indexValue = interpreter->evaluate(*node.index);

  if (auto *array = std::get_if<ArrayValue>(&arrayValue))
  {
    if (auto *index = std::get_if<int>(&indexValue))
    {
      if (*index >= 0 && *index < static_cast<int>(array->elements->size()))
      {
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
void EvalVisitor::visit(ExpressionStmt &node)
{
  result = interpreter->evaluate(*node.expression);
}

void EvalVisitor::visit(VariableDeclStmt &node)
{
  (void)node;
  result = Value();
}

void EvalVisitor::visit(FunctionDeclStmt &node)
{
  (void)node;
  result = Value();
}

void EvalVisitor::visit(BlockStmt &node)
{
  (void)node;
  result = Value();
}

void EvalVisitor::visit(IfStmt &node)
{
  (void)node;
  result = Value();
}

void EvalVisitor::visit(ReturnStmt &node)
{
  (void)node;
  result = Value();
}

void EvalVisitor::visit(ClassDeclStmt &node)
{
  (void)node;
  result = Value();
}

void EvalVisitor::visit(ForStmt &node)
{
  // Evaluate the iterable expression
  Value iterableValue = interpreter->evaluate(*node.iterable);

  // Check if it's an array
  if (auto *arrayValue = std::get_if<ArrayValue>(&iterableValue))
  {
    // Create a new scope for the loop variable
    auto loopScope = std::make_shared<Environment>(interpreter->environment);

    // Iterate through array elements
    for (const auto &element : *arrayValue->elements)
    {
      // Set the loop variable in the new scope
      loopScope->define(node.variable, element);

      // Temporarily switch to loop scope
      auto oldEnv = interpreter->environment;
      interpreter->environment = loopScope;

      try
      {
        // Execute the loop body
        interpreter->execute(*node.body);
      }
      catch (...)
      {
        // Restore environment on exception
        interpreter->environment = oldEnv;
        throw;
      }

      // Restore environment
      interpreter->environment = oldEnv;
    }
  }
  else
  {
    throw std::runtime_error("Can only iterate over arrays");
  }

  result = Value();
}

void EvalVisitor::visit(WhenStmt &node)
{
  // Evaluate the subject expression
  Value subjectValue = interpreter->evaluate(*node.subject);

  // Check each branch
  for (const auto &branch : node.branches)
  {
    Value conditionValue = interpreter->evaluate(*branch.first);

    // Check if condition matches subject
    bool matches = false;
    if (auto *subjectInt = std::get_if<int>(&subjectValue))
    {
      if (auto *conditionInt = std::get_if<int>(&conditionValue))
      {
        matches = (*subjectInt == *conditionInt);
      }
    }
    else if (auto *subjectStr = std::get_if<std::string>(&subjectValue))
    {
      if (auto *conditionStr = std::get_if<std::string>(&conditionValue))
      {
        matches = (*subjectStr == *conditionStr);
      }
    }

    if (matches)
    {
      // Execute the matching branch
      interpreter->execute(*branch.second);
      result = Value();
      return;
    }
  }

  // Check for else branch
  if (node.elseBranch)
  {
    interpreter->execute(**node.elseBranch);
  }

  result = Value();
}

void EvalVisitor::visit(TryStmt &node)
{
  (void)node;
  result = Value();
}

void EvalVisitor::visit(ConstructorDeclStmt &node)
{
  (void)node;
  result = Value();
}

void EvalVisitor::visit(WhileStmt &node)
{
  (void)node;
  result = Value();
}

void EvalVisitor::visit(ExtensionFunctionDeclStmt &node)
{
  (void)node;
  result = Value();
}
