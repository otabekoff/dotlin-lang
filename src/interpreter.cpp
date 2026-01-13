#include "dotlin/interpreter.h"
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace dotlin
{

  void Environment::define(const std::string &name, Value value)
  {
    values[name] = value;
  }

  Value Environment::get(const std::string &name)
  {
    auto it = values.find(name);
    if (it != values.end())
    {
      return it->second;
    }

    if (enclosing)
    {
      return enclosing->get(name);
    }

    throw std::runtime_error("Undefined variable: " + name);
  }

  void Environment::assign(const std::string &name, Value value)
  {
    auto it = values.find(name);
    if (it != values.end())
    {
      values[name] = value;
      return;
    }

    if (enclosing)
    {
      enclosing->assign(name, value);
      return;
    }

    throw std::runtime_error("Undefined variable: " + name);
  }

  Interpreter::Interpreter() : environment(&globals), hasMainFunction(false), mainFunctionStmt(nullptr), commandLineArgs({}) {}

  Value Interpreter::interpret(const Program &program)
  {
    std::vector<std::string> empty_args;
    return interpret(program, empty_args);
  }

  Value Interpreter::interpret(const Program &program, const std::vector<std::string> &args)
  {
    // Store command-line arguments
    commandLineArgs = args;

    // First, execute all statements to register functions and declare variables
    for (const auto &stmt : program.statements)
    {
      execute(*stmt);
    }

    // For now, return a dummy value
    return std::string("Program executed successfully");
  }

  Value interpret(const Program &program)
  {
    std::vector<std::string> empty_args;
    Interpreter interpreter;
    return interpreter.interpret(program, empty_args);
  }

  Value interpret(const Program &program, const std::vector<std::string> &args)
  {
    Interpreter interpreter;
    return interpreter.interpret(program, args);
  }

  // Visitor pattern implementations
  void Interpreter::visit(LiteralExpr &node)
  {
    // The literal value is already stored in the node.value
    // Using the node to avoid unused parameter warning
    (void)node;
  }

  void Interpreter::visit(IdentifierExpr &node)
  {
    // Look up identifier in environment
    // In a real implementation, this would return the value from the environment
    (void)node;
  }

  void Interpreter::visit(BinaryExpr &node)
  {
    // Evaluate binary expressions
    auto leftValue = evaluate(*node.left);
    auto rightValue = evaluate(*node.right);

    // Perform the operation based on node.op
    switch (node.op)
    {
    case TokenType::PLUS:
      // Would need to handle different types properly
      break;
    case TokenType::MINUS:
      // Would need to handle different types properly
      break;
    case TokenType::MULTIPLY:
      // Would need to handle different types properly
      break;
    case TokenType::DIVIDE:
      // Would need to handle different types properly
      break;
    case TokenType::EQUAL:
      // Would need to handle comparison
      break;
    default:
      break;
    }
  }

  void Interpreter::visit(UnaryExpr &node)
  {
    // Evaluate unary expressions
    auto operandValue = evaluate(*node.operand);

    // Perform the operation based on node.op
    switch (node.op)
    {
    case TokenType::NOT:
      // Handle logical NOT
      break;
    case TokenType::MINUS:
      // Handle unary minus
      break;
    default:
      break;
    }
  }

  void Interpreter::visit(CallExpr &node)
  {
    // Handle function calls
    // For statement execution, we primarily care about side effects like printing
    std::cout << "DEBUG: In CallExpr evaluator (statement)" << std::endl;
    std::cout << "DEBUG: Number of args: " << node.arguments.size() << std::endl;

    // Check if this is a built-in function call
    if (auto *identifier = dynamic_cast<IdentifierExpr *>(node.callee.get()))
    {
      std::string funcName = identifier->name;
      if (funcName == "println")
      {
        if (!node.arguments.empty())
        {
          auto argValue = evaluate(*node.arguments[0]);
          std::cout << valueToString(argValue);
        }
        std::cout << std::endl; // Add newline for println
        return;
      }
      else if (funcName == "print")
      {
        if (!node.arguments.empty())
        {
          auto argValue = evaluate(*node.arguments[0]);
          std::cout << valueToString(argValue);
        }
        return;
      }
      else if (funcName == "readln")
      {
        std::string input;
        std::getline(std::cin, input);
        // Store the result somehow - for now just return
        return;
      }
    }

    // For other function calls, evaluate arguments but don't do anything
    for (const auto &arg : node.arguments)
    {
      evaluate(*arg);
    }
  }

  void Interpreter::visit(MemberAccessExpr &node)
  {
    // Handle member access for statement execution
    // We need to evaluate the expression to trigger any side effects
    evaluate(node);
  }

  void Interpreter::visit(VariableDeclStmt &node)
  {
    Value value;
    if (node.initializer)
    {
      // Evaluate the initializer expression
      value = evaluate(**node.initializer);
    }
    else
    {
      // Default value for uninitialized variables
      value = 0; // Default to integer 0 for now
    }

    environment->define(node.name, value);
  }

  void Interpreter::visit(FunctionDeclStmt &node)
  {
    // Handle function declarations by storing them in the environment
    if (node.name == "main")
    {
      // This is the main function - execute its body immediately
      Environment *previousEnv = environment;
      Environment mainEnv(&globals); // Create environment for main function
      environment = &mainEnv;

      // If main function has parameters, initialize them with command-line arguments
      for (size_t i = 0; i < node.parameters.size(); ++i)
      {
        if (i < commandLineArgs.size())
        {
          // Initialize parameter with command-line argument
          environment->define(node.parameters[i], commandLineArgs[i]);
        }
        else
        {
          // Initialize parameter with empty/default value
          environment->define(node.parameters[i], std::string(""));
        }
      }

      // Execute the main function body (which is a single statement)
      if (node.body)
      {
        execute(*node.body);
      }

      environment = previousEnv; // Restore previous environment
    }
    // In a complete implementation, we would store the function in the environment
    // so it can be called later
  }

  void Interpreter::visit(BlockStmt &node)
  {
    // Execute block with new environment
    Environment blockEnv(environment);
    Environment *previousEnv = environment;
    environment = &blockEnv;

    for (const auto &stmt : node.statements)
    {
      execute(*stmt);
    }

    environment = previousEnv;
  }

  void Interpreter::visit(ReturnStmt &node)
  {
    // Handle return statements
    // In a real implementation, would set return value and exit function
    (void)node;
  }

  void Interpreter::visit(IfStmt &node)
  {
    // Handle if statements
    auto conditionValue = evaluate(*node.condition);

    // In a real implementation, would execute appropriate branch based on
    // condition
    if (std::holds_alternative<bool>(conditionValue) &&
        std::get<bool>(conditionValue))
    {
      execute(*node.thenBranch);
    }
    else if (node.elseBranch)
    {
      execute(**node.elseBranch);
    }
  }

  void Interpreter::visit(ExpressionStmt &node)
  {
    // Evaluate the expression but ignore the result (like in most languages)
    // Expression statements are typically used for side effects
    evaluate(*node.expression);
  }

  Value Interpreter::evaluate(Expression &expr)
  {
    std::cout << "DEBUG: Starting expression evaluation" << std::endl;
    // Use the visitor pattern to dispatch to the appropriate visit method
    struct EvalVisitor : public AstVisitor
    {
      Value result;
      Interpreter *interpreter;

      EvalVisitor(Interpreter *interp) : interpreter(interp) {}

      void visit(LiteralExpr &node) override
      {
        // Return the literal value directly
        std::cout << "DEBUG: In LiteralExpr evaluator" << std::endl;
        result = node.value;
      }

      void visit(IdentifierExpr &node) override
      {
        // Special handling for "args" variable
        if (node.name == "args")
        {
          // Create an array-like representation of command-line arguments
          // For now, return a placeholder string representing args array
          std::vector<Value> argsVector;
          for (const auto &arg : interpreter->commandLineArgs)
          {
            argsVector.push_back(arg);
          }
          // For now, return a placeholder since we don't have array type yet
          result = std::string("args_array");
        }
        // Look up the value in the environment
        else
        {
          try
          {
            result = interpreter->environment->get(node.name);
          }
          catch (const std::exception &)
          {
            // Return a default value if variable not found
            result = std::string("undefined");
          }
        }
      }

      void visit(BinaryExpr &node) override
      {
        // Evaluate operands and perform operation
        auto leftVal = interpreter->evaluate(*node.left);
        auto rightVal = interpreter->evaluate(*node.right);

        // Handle different type combinations for operations
        switch (node.op)
        {
        case TokenType::PLUS:
          // String concatenation or numeric addition
          if (std::holds_alternative<std::string>(leftVal) ||
              std::holds_alternative<std::string>(rightVal))
          {
            std::string leftStr, rightStr;
            if (std::holds_alternative<int>(leftVal))
            {
              leftStr = std::to_string(std::get<int>(leftVal));
            }
            else if (std::holds_alternative<double>(leftVal))
            {
              leftStr = std::to_string(std::get<double>(leftVal));
            }
            else if (std::holds_alternative<bool>(leftVal))
            {
              leftStr = std::get<bool>(leftVal) ? "true" : "false";
            }
            else
            {
              leftStr = std::get<std::string>(leftVal);
            }
            if (std::holds_alternative<int>(rightVal))
            {
              rightStr = std::to_string(std::get<int>(rightVal));
            }
            else if (std::holds_alternative<double>(rightVal))
            {
              rightStr = std::to_string(std::get<double>(rightVal));
            }
            else if (std::holds_alternative<bool>(rightVal))
            {
              rightStr = std::get<bool>(rightVal) ? "true" : "false";
            }
            else
            {
              rightStr = std::get<std::string>(rightVal);
            }
            result = leftStr + rightStr;
          }
          else if (std::holds_alternative<int>(leftVal) &&
                   std::holds_alternative<int>(rightVal))
          {
            result = std::get<int>(leftVal) + std::get<int>(rightVal);
          }
          else if (std::holds_alternative<double>(leftVal) ||
                   std::holds_alternative<double>(rightVal))
          {
            double left = std::holds_alternative<int>(leftVal)
                              ? static_cast<double>(std::get<int>(leftVal))
                              : std::get<double>(leftVal);
            double right = std::holds_alternative<int>(rightVal)
                               ? static_cast<double>(std::get<int>(rightVal))
                               : std::get<double>(rightVal);
            result = left + right;
          }
          else
          {
            result = 0; // Default for unsupported operations
          }
          break;
        case TokenType::MINUS:
          if (std::holds_alternative<int>(leftVal) &&
              std::holds_alternative<int>(rightVal))
          {
            result = std::get<int>(leftVal) - std::get<int>(rightVal);
          }
          else if (std::holds_alternative<double>(leftVal) ||
                   std::holds_alternative<double>(rightVal))
          {
            double left = std::holds_alternative<int>(leftVal)
                              ? static_cast<double>(std::get<int>(leftVal))
                              : std::get<double>(leftVal);
            double right = std::holds_alternative<int>(rightVal)
                               ? static_cast<double>(std::get<int>(rightVal))
                               : std::get<double>(rightVal);
            result = left - right;
          }
          else
          {
            result = 0; // Default for unsupported operations
          }
          break;
        case TokenType::MULTIPLY:
          if (std::holds_alternative<int>(leftVal) &&
              std::holds_alternative<int>(rightVal))
          {
            result = std::get<int>(leftVal) * std::get<int>(rightVal);
          }
          else if (std::holds_alternative<double>(leftVal) ||
                   std::holds_alternative<double>(rightVal))
          {
            double left = std::holds_alternative<int>(leftVal)
                              ? static_cast<double>(std::get<int>(leftVal))
                              : std::get<double>(leftVal);
            double right = std::holds_alternative<int>(rightVal)
                               ? static_cast<double>(std::get<int>(rightVal))
                               : std::get<double>(rightVal);
            result = left * right;
          }
          else
          {
            result = 0; // Default for unsupported operations
          }
          break;
        case TokenType::DIVIDE:
          if (std::holds_alternative<int>(leftVal) &&
              std::holds_alternative<int>(rightVal))
          {
            result = std::get<int>(rightVal) != 0
                         ? std::get<int>(leftVal) / std::get<int>(rightVal)
                         : 0;
          }
          else if (std::holds_alternative<double>(leftVal) ||
                   std::holds_alternative<double>(rightVal))
          {
            double left = std::holds_alternative<int>(leftVal)
                              ? static_cast<double>(std::get<int>(leftVal))
                              : std::get<double>(leftVal);
            double right = std::holds_alternative<int>(rightVal)
                               ? static_cast<double>(std::get<int>(rightVal))
                               : std::get<double>(rightVal);
            result = right != 0 ? left / right : 0.0;
          }
          else
          {
            result = 0; // Default for unsupported operations
          }
          break;
        case TokenType::MODULO:
          if (std::holds_alternative<int>(leftVal) &&
              std::holds_alternative<int>(rightVal))
          {
            result = std::get<int>(rightVal) != 0
                         ? std::get<int>(leftVal) % std::get<int>(rightVal)
                         : 0;
          }
          else
          {
            result = 0; // Default for unsupported operations
          }
          break;
        case TokenType::EQUAL:
          // Equality comparison for different types
          if (std::holds_alternative<int>(leftVal) &&
              std::holds_alternative<int>(rightVal))
          {
            result = std::get<int>(leftVal) == std::get<int>(rightVal);
          }
          else if (std::holds_alternative<double>(leftVal) ||
                   std::holds_alternative<double>(rightVal))
          {
            double left = std::holds_alternative<int>(leftVal)
                              ? static_cast<double>(std::get<int>(leftVal))
                              : std::get<double>(leftVal);
            double right = std::holds_alternative<int>(rightVal)
                               ? static_cast<double>(std::get<int>(rightVal))
                               : std::get<double>(rightVal);
            result = left == right;
          }
          else if (std::holds_alternative<std::string>(leftVal) &&
                   std::holds_alternative<std::string>(rightVal))
          {
            result =
                std::get<std::string>(leftVal) == std::get<std::string>(rightVal);
          }
          else if (std::holds_alternative<bool>(leftVal) &&
                   std::holds_alternative<bool>(rightVal))
          {
            result = std::get<bool>(leftVal) == std::get<bool>(rightVal);
          }
          else
          {
            result = false; // Default for unsupported comparisons
          }
          break;
        case TokenType::NOT_EQUAL:
          if (std::holds_alternative<int>(leftVal) &&
              std::holds_alternative<int>(rightVal))
          {
            result = std::get<int>(leftVal) != std::get<int>(rightVal);
          }
          else if (std::holds_alternative<double>(leftVal) ||
                   std::holds_alternative<double>(rightVal))
          {
            double left = std::holds_alternative<int>(leftVal)
                              ? static_cast<double>(std::get<int>(leftVal))
                              : std::get<double>(leftVal);
            double right = std::holds_alternative<int>(rightVal)
                               ? static_cast<double>(std::get<int>(rightVal))
                               : std::get<double>(rightVal);
            result = left != right;
          }
          else if (std::holds_alternative<std::string>(leftVal) &&
                   std::holds_alternative<std::string>(rightVal))
          {
            result =
                std::get<std::string>(leftVal) != std::get<std::string>(rightVal);
          }
          else if (std::holds_alternative<bool>(leftVal) &&
                   std::holds_alternative<bool>(rightVal))
          {
            result = std::get<bool>(leftVal) != std::get<bool>(rightVal);
          }
          else
          {
            result = true; // Default for unsupported comparisons
          }
          break;
        case TokenType::LESS:
          if (std::holds_alternative<int>(leftVal) &&
              std::holds_alternative<int>(rightVal))
          {
            result = std::get<int>(leftVal) < std::get<int>(rightVal);
          }
          else if (std::holds_alternative<double>(leftVal) ||
                   std::holds_alternative<double>(rightVal))
          {
            double left = std::holds_alternative<int>(leftVal)
                              ? static_cast<double>(std::get<int>(leftVal))
                              : std::get<double>(leftVal);
            double right = std::holds_alternative<int>(rightVal)
                               ? static_cast<double>(std::get<int>(rightVal))
                               : std::get<double>(rightVal);
            result = left < right;
          }
          else
          {
            result = false; // Default for unsupported comparisons
          }
          break;
        case TokenType::LESS_EQUAL:
          if (std::holds_alternative<int>(leftVal) &&
              std::holds_alternative<int>(rightVal))
          {
            result = std::get<int>(leftVal) <= std::get<int>(rightVal);
          }
          else if (std::holds_alternative<double>(leftVal) ||
                   std::holds_alternative<double>(rightVal))
          {
            double left = std::holds_alternative<int>(leftVal)
                              ? static_cast<double>(std::get<int>(leftVal))
                              : std::get<double>(leftVal);
            double right = std::holds_alternative<int>(rightVal)
                               ? static_cast<double>(std::get<int>(rightVal))
                               : std::get<double>(rightVal);
            result = left <= right;
          }
          else
          {
            result = false; // Default for unsupported comparisons
          }
          break;
        case TokenType::GREATER:
          if (std::holds_alternative<int>(leftVal) &&
              std::holds_alternative<int>(rightVal))
          {
            result = std::get<int>(leftVal) > std::get<int>(rightVal);
          }
          else if (std::holds_alternative<double>(leftVal) ||
                   std::holds_alternative<double>(rightVal))
          {
            double left = std::holds_alternative<int>(leftVal)
                              ? static_cast<double>(std::get<int>(leftVal))
                              : std::get<double>(leftVal);
            double right = std::holds_alternative<int>(rightVal)
                               ? static_cast<double>(std::get<int>(rightVal))
                               : std::get<double>(rightVal);
            result = left > right;
          }
          else
          {
            result = false; // Default for unsupported comparisons
          }
          break;
        case TokenType::GREATER_EQUAL:
          if (std::holds_alternative<int>(leftVal) &&
              std::holds_alternative<int>(rightVal))
          {
            result = std::get<int>(leftVal) >= std::get<int>(rightVal);
          }
          else if (std::holds_alternative<double>(leftVal) ||
                   std::holds_alternative<double>(rightVal))
          {
            double left = std::holds_alternative<int>(leftVal)
                              ? static_cast<double>(std::get<int>(leftVal))
                              : std::get<double>(leftVal);
            double right = std::holds_alternative<int>(rightVal)
                               ? static_cast<double>(std::get<int>(rightVal))
                               : std::get<double>(rightVal);
            result = left >= right;
          }
          else
          {
            result = false; // Default for unsupported comparisons
          }
          break;
        case TokenType::AND:
          if (std::holds_alternative<bool>(leftVal) &&
              std::holds_alternative<bool>(rightVal))
          {
            result = std::get<bool>(leftVal) && std::get<bool>(rightVal);
          }
          else
          {
            result = false; // Default for unsupported operations
          }
          break;
        case TokenType::OR:
          if (std::holds_alternative<bool>(leftVal) &&
              std::holds_alternative<bool>(rightVal))
          {
            result = std::get<bool>(leftVal) || std::get<bool>(rightVal);
          }
          else
          {
            result = false; // Default for unsupported operations
          }
          break;
        default:
          result = 0; // Default for unsupported operations
          break;
        }
      }

      void visit(UnaryExpr &node) override
      {
        auto operandVal = interpreter->evaluate(*node.operand);

        switch (node.op)
        {
        case TokenType::MINUS:
          if (std::holds_alternative<int>(operandVal))
          {
            result = -std::get<int>(operandVal);
          }
          else if (std::holds_alternative<double>(operandVal))
          {
            result = -std::get<double>(operandVal);
          }
          else
          {
            result = 0; // Default for unsupported operations
          }
          break;
        case TokenType::NOT:
          if (std::holds_alternative<bool>(operandVal))
          {
            result = !std::get<bool>(operandVal);
          }
          else if (std::holds_alternative<int>(operandVal))
          {
            // For integers, treat 0 as false and non-zero as true
            result = !std::get<int>(operandVal);
          }
          else
          {
            result = false; // Default for unsupported operations
          }
          break;
        case TokenType::INCREMENT:
          if (std::holds_alternative<int>(operandVal))
          {
            result = std::get<int>(operandVal) + 1;
          }
          else if (std::holds_alternative<double>(operandVal))
          {
            result = std::get<double>(operandVal) + 1.0;
          }
          else
          {
            result = 1; // Default for unsupported operations
          }
          break;
        case TokenType::DECREMENT:
          if (std::holds_alternative<int>(operandVal))
          {
            result = std::get<int>(operandVal) - 1;
          }
          else if (std::holds_alternative<double>(operandVal))
          {
            result = std::get<double>(operandVal) - 1.0;
          }
          else
          {
            result = -1; // Default for unsupported operations
          }
          break;
        default:
          result = operandVal; // Return as-is for other types
          break;
        }
      }

      void visit(CallExpr &node) override
      {
        // Handle function calls
        std::cout << "DEBUG: In CallExpr evaluator" << std::endl;
        std::vector<Value> args;
        for (auto &arg : node.arguments)
        {
          args.push_back(interpreter->evaluate(*arg));
        }

        std::cout << "DEBUG: Number of args: " << args.size() << std::endl;

        // Check if it's a method call on an object (callee is MemberAccessExpr)
        if (auto *memberAccess = dynamic_cast<MemberAccessExpr *>(node.callee.get()))
        {
          // Determine if this is a direct method call (str.substring()) or a method on result (args.size().toString())
          if (auto *ident = dynamic_cast<IdentifierExpr *>(memberAccess->object.get()))
          {
            // This is a direct method call like str.substring() or str.indexOf()
            std::string objName = ident->name;
            std::string methodName = memberAccess->property;

            // Get the object value first
            Value objValue = interpreter->evaluate(*ident);

            if (methodName == "substring" && args.size() == 1 && std::holds_alternative<int>(args[0]))
            {
              // Handle substring(start) method call
              if (std::holds_alternative<std::string>(objValue))
              {
                std::string str = std::get<std::string>(objValue);
                int start = std::get<int>(args[0]);
                if (start >= 0 && start <= static_cast<int>(str.length()))
                {
                  result = str.substr(start);
                }
                else
                {
                  result = std::string(""); // Return empty string for invalid index
                }
              }
              return;
            }
            else if (methodName == "substring" && args.size() == 2 &&
                     std::holds_alternative<int>(args[0]) && std::holds_alternative<int>(args[1]))
            {
              // Handle substring(start, end) method call
              if (std::holds_alternative<std::string>(objValue))
              {
                std::string str = std::get<std::string>(objValue);
                int start = std::get<int>(args[0]);
                int end = std::get<int>(args[1]);
                if (start >= 0 && end <= static_cast<int>(str.length()) && start <= end)
                {
                  result = str.substr(start, end - start);
                }
                else
                {
                  result = std::string(""); // Return empty string for invalid range
                }
              }
              return;
            }
            else if (methodName == "indexOf" && args.size() == 1 && std::holds_alternative<std::string>(args[0]))
            {
              // Handle indexOf(substring) method call
              if (std::holds_alternative<std::string>(objValue))
              {
                std::string str = std::get<std::string>(objValue);
                std::string substr = std::get<std::string>(args[0]);
                size_t pos = str.find(substr);
                result = static_cast<int>(pos != std::string::npos ? pos : -1);
              }
              return;
            }
          }
          else
          {
            // This is a method call on the result of a member access, like args.size().toString()
            // Evaluate the member access expression to get the value
            Value objValue = interpreter->evaluate(*memberAccess);

            // For now, we mainly handle toString() on any value
            if (args.empty()) // Assume this is toString() with no arguments
            {
              result = interpreter->valueToString(objValue);
              return;
            }
          }
          // Add other method implementations here as needed
        }

        // Check if it's a built-in function like println
        if (auto *ident = dynamic_cast<IdentifierExpr *>(node.callee.get()))
        {
          if (ident->name == "println")
          {
            if (!args.empty())
            {
              // Convert the argument to string and print it
              std::string output = interpreter->valueToString(args[0]);
              std::cout << output << std::endl;
            }
            else
            {
              std::cout << std::endl;
            }
            result = 0; // Return a default value
          }
          else if (ident->name == "print")
          {
            if (!args.empty())
            {
              // Convert the argument to string and print it without newline
              std::string output = interpreter->valueToString(args[0]);
              std::cout << output;
            }
            result = 0; // Return a default value
          }
          else if (ident->name == "readln")
          {
            // Read a line from standard input
            std::string input;
            std::getline(std::cin, input);
            result = input;
          }
          else
          {
            // For now, return a placeholder for other function calls
            result = std::string("call_result");
          }
        }
        else
        {
          // For complex expressions as callee, just return a placeholder
          // This could be method calls or other complex call expressions
          result = std::string("complex_call_result");
        }
      }

      void visit(MemberAccessExpr &node) override
      {
        // Handle member access
        auto objValue = interpreter->evaluate(*node.object);

        // Check if the object is an identifier "args" and the property is "size"
        if (auto *identifier = dynamic_cast<IdentifierExpr *>(node.object.get()))
        {
          if (identifier->name == "args" && node.property == "size")
          {
            // Return the size of command-line arguments
            result = static_cast<int>(interpreter->commandLineArgs.size());
            return;
          }
        }

        if (node.property == "length" && std::holds_alternative<std::string>(objValue))
        {
          // Handle .length property for strings
          std::string str = std::get<std::string>(objValue);
          result = static_cast<int>(str.length()); // Return the length as an integer
        }
        else if (node.property == "toString")
        {
          // Handle .toString() method for all types
          result = interpreter->valueToString(objValue);
        }
        else if (node.property == "substring" && std::holds_alternative<std::string>(objValue))
        {
          // Handle .substring() property access without arguments
          // For now, return a placeholder - substring with arguments is handled in CallExpr
          result = std::string("");
        }
        else if (node.property == "indexOf" && std::holds_alternative<std::string>(objValue))
        {
          // Handle .indexOf property access without arguments
          // For now, return a placeholder - indexOf with arguments is handled in CallExpr
          result = std::string("");
        }
        else
        {
          // For now, return a placeholder for other member accesses
          result = std::string("member_access_result");
        }
      }

      void visit(ArrayAccessExpr &node) override
      {
        // Handle array access like args[0]
        auto arrayValue = interpreter->evaluate(*node.array);
        auto indexValue = interpreter->evaluate(*node.index);

        // Check if we're accessing the args array
        if (std::holds_alternative<std::string>(arrayValue) &&
            std::get<std::string>(arrayValue) == "args_array" &&
            std::holds_alternative<int>(indexValue))
        {
          int index = std::get<int>(indexValue);
          if (index >= 0 && static_cast<size_t>(index) < interpreter->commandLineArgs.size())
          {
            result = interpreter->commandLineArgs[index];
          }
          else
          {
            result = std::string("index_out_of_bounds");
          }
        }
        else
        {
          result = std::string("unsupported_array_access");
        }
      }

      // Statement visitor methods (required by interface but shouldn't be called for expressions)
      void visit(ExpressionStmt &node) override
      {
        result = std::string("error: expression visitor called on statement");
        (void)node;
      }
      void visit(VariableDeclStmt &node) override
      {
        result = std::string("error: expression visitor called on statement");
        (void)node;
      }
      void visit(FunctionDeclStmt &node) override
      {
        result = std::string("error: expression visitor called on statement");
        (void)node;
      }
      void visit(BlockStmt &node) override
      {
        result = std::string("error: expression visitor called on statement");
        (void)node;
      }
      void visit(ReturnStmt &node) override
      {
        result = std::string("error: expression visitor called on statement");
        (void)node;
      }
      void visit(IfStmt &node) override
      {
        result = std::string("error: expression visitor called on statement");
        (void)node;
      }
    };

    EvalVisitor visitor(this);
    try
    {
      expr.accept(visitor);
    }
    catch (const std::exception &e)
    {
      std::cout << "Exception in expression evaluation: " << e.what() << std::endl;
    }
    return visitor.result;
  }

  void Interpreter::execute(Statement &stmt)
  {
    // Use the visitor pattern to dispatch to the appropriate visit method
    struct ExecVisitor : public AstVisitor
    {
      Interpreter *interpreter;

      ExecVisitor(Interpreter *interp) : interpreter(interp) {}

      void visit(LiteralExpr &node) override { (void)node; }
      void visit(IdentifierExpr &node) override { (void)node; }
      void visit(BinaryExpr &node) override { (void)node; }
      void visit(UnaryExpr &node) override { (void)node; }
      void visit(CallExpr &node) override
      {
        // Evaluate the member access expression
        interpreter->evaluate(node);
      }
      void visit(MemberAccessExpr &node) override
      {
        // Evaluate the member access expression
        interpreter->evaluate(node);
      }
      void visit(ArrayAccessExpr &node) override
      {
        // Evaluate the array access expression
        interpreter->evaluate(node);
      }
      void visit(ExpressionStmt &node) override { interpreter->visit(node); }
      void visit(VariableDeclStmt &node) override { interpreter->visit(node); }
      void visit(FunctionDeclStmt &node) override { interpreter->visit(node); }
      void visit(BlockStmt &node) override { interpreter->visit(node); }
      void visit(ReturnStmt &node) override { interpreter->visit(node); }
      void visit(IfStmt &node) override { interpreter->visit(node); }
    };

    ExecVisitor visitor(this);
    stmt.accept(visitor);
  }

  std::string Interpreter::valueToString(const Value &value)
  {
    if (std::holds_alternative<int>(value))
    {
      return std::to_string(std::get<int>(value));
    }
    else if (std::holds_alternative<double>(value))
    {
      return std::to_string(std::get<double>(value));
    }
    else if (std::holds_alternative<bool>(value))
    {
      return std::get<bool>(value) ? "true" : "false";
    }
    else if (std::holds_alternative<std::string>(value))
    {
      return std::get<std::string>(value);
    }
    else
    {
      return "undefined";
    }
  }

  Value Interpreter::executeBlock(const std::vector<Statement::Ptr> &statements,
                                  Environment *env)
  {
    Environment *previousEnv = environment;
    environment = env;

    Value result;
    for (const auto &stmt : statements)
    {
      execute(*stmt);
    }

    environment = previousEnv;
    return result;
  }

} // namespace dotlin