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

  Interpreter::Interpreter() : globals(std::make_shared<Environment>()), environment(globals), functionEnvironment(nullptr), hasMainFunction(false), mainFunctionStmt(nullptr), commandLineArgs({}) {}

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
    return Value(std::string("Program executed successfully"));
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
    // This method is for statement execution, not expression evaluation
    // For expression evaluation, the visitor in evaluate() method is used
    (void)node;
  }

  void Interpreter::visit(BinaryExpr &node)
  {
    // Handle binary expressions based on operation type
    switch (node.op)
    {
    case TokenType::ASSIGN:
      // Handle assignment: left operand should be an identifier
      if (auto *identifier = dynamic_cast<IdentifierExpr *>(node.left.get()))
      {
        // Evaluate the right-hand side
        auto rightVal = node.right ? evaluate(*node.right) : Value(std::string("null"));

        std::cout << "DEBUG: Assignment statement - trying to assign " << identifier->name << " = ";
        std::cout << valueToString(rightVal) << std::endl;

        // Try to assign to the variable in the current environment first
        try
        {
          environment->assign(identifier->name, rightVal);
          std::cout << "DEBUG: Assignment statement successful in current environment" << std::endl;
        }
        catch (const std::exception &e)
        {
          std::cout << "DEBUG: Assignment statement failed in current environment: " << e.what() << std::endl;
          // If assignment fails in current environment, try the function environment
          if (functionEnvironment && functionEnvironment != environment)
          {
            try
            {
              functionEnvironment->assign(identifier->name, rightVal);
              std::cout << "DEBUG: Assignment statement successful in function environment" << std::endl;
            }
            catch (const std::exception &e2)
            {
              std::cout << "DEBUG: Assignment statement failed in function environment: " << e2.what() << std::endl;
              // If assignment fails everywhere, define in the current environment
              environment->define(identifier->name, rightVal);
              std::cout << "DEBUG: Defined new variable in current environment" << std::endl;
            }
          }
          else
          {
            // If no function environment or it's the same as current, define in current
            environment->define(identifier->name, rightVal);
            std::cout << "DEBUG: Defined new variable in current environment" << std::endl;
          }
        }
      }
      break;
    case TokenType::PLUS:
      // For arithmetic operations in statements, evaluate both sides
      evaluate(*node.left);
      evaluate(*node.right);
      break;
    case TokenType::MINUS:
      evaluate(*node.left);
      evaluate(*node.right);
      break;
    case TokenType::MULTIPLY:
      evaluate(*node.left);
      evaluate(*node.right);
      break;
    case TokenType::DIVIDE:
      evaluate(*node.left);
      evaluate(*node.right);
      break;
    case TokenType::EQUAL:
      // For comparison in statements, evaluate both sides
      evaluate(*node.left);
      evaluate(*node.right);
      break;
    default:
      // For other operations, just evaluate both sides
      if (node.left)
        evaluate(*node.left);
      if (node.right)
        evaluate(*node.right);
      break;
    }
  }

  void Interpreter::visit(UnaryExpr &node)
  {
    // Evaluate unary expressions
    auto operandValue = node.operand ? evaluate(*node.operand) : Value(std::string("null"));

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
        if (!node.arguments.empty() && node.arguments[0])
        {
          auto argValue = evaluate(*node.arguments[0]);
          std::cout << valueToString(argValue);
        }
        std::cout << std::endl; // Add newline for println
        return;
      }
      else if (funcName == "print")
      {
        if (!node.arguments.empty() && node.arguments[0])
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
      if (arg)
      {
        evaluate(*arg);
      }
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
      value = node.initializer ? evaluate(**node.initializer) : Value(0);
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
    // Store function in environment for later use
    // For now, we'll just store a marker that the function exists
    // The actual function calling mechanism would need more sophisticated handling
    environment->define(node.name, Value(std::string("function_defined")));

    // Special handling for main function
    if (node.name == "main")
    {
      // Execute main function if it exists
      auto previousEnv = environment;
      auto previousFuncEnv = functionEnvironment;
      environment = std::make_shared<Environment>(globals, false); // Create environment for main function, not a block scope
      functionEnvironment = environment;                           // Set the function environment

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

      // Execute the main function body
      if (node.body)
      {
        // If the body is a block (which it typically is), execute its statements directly
        // in the function environment to avoid creating an extra block scope level
        if (auto *blockBody = dynamic_cast<BlockStmt *>(node.body.get()))
        {
          for (const auto &stmt : blockBody->statements)
          {
            if (stmt)
              execute(*stmt);
          }
        }
        else
        {
          // For non-block bodies, execute normally
          execute(*node.body);
        }
      }

      environment = previousEnv;             // Restore previous environment
      functionEnvironment = previousFuncEnv; // Restore previous function environment
    }
  }

  void Interpreter::visit(BlockStmt &node)
  {
    // Execute block with new environment that marks this as a block scope
    executeBlock(node.statements, environment);
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
    auto conditionValue = node.condition ? evaluate(*node.condition) : Value(false);

    // In a real implementation, would execute appropriate branch based on
    // condition
    if (std::holds_alternative<bool>(conditionValue) &&
        std::get<bool>(conditionValue))
    {
      if (node.thenBranch)
        execute(*node.thenBranch);
    }
    else if (node.elseBranch.has_value() && node.elseBranch.value())
    {
      execute(*node.elseBranch.value());
    }
  }

  void Interpreter::visit(WhileStmt &node)
  {
    // Execute the while loop
    while (true)
    {
      // Evaluate the condition
      auto conditionValue = node.condition ? evaluate(*node.condition) : Value(false);

      // Check if the condition is true
      bool shouldContinue = false;
      if (std::holds_alternative<bool>(conditionValue))
      {
        shouldContinue = std::get<bool>(conditionValue);
      }
      else if (std::holds_alternative<int>(conditionValue))
      {
        // Treat non-zero integers as true
        shouldContinue = std::get<int>(conditionValue) != 0;
      }
      else if (std::holds_alternative<std::string>(conditionValue))
      {
        // Treat non-empty strings as true
        shouldContinue = !std::get<std::string>(conditionValue).empty();
      }

      if (!shouldContinue)
      {
        break; // Exit the loop if condition is false
      }

      // Execute the loop body
      if (node.body)
      {
        // If the body is a block, execute its statements directly in the current environment
        // so that variable changes persist across iterations
        if (auto *blockBody = dynamic_cast<BlockStmt *>(node.body.get()))
        {
          for (const auto &stmt : blockBody->statements)
          {
            if (stmt)
              execute(*stmt);
          }
        }
        else
        {
          // For non-block bodies, execute normally
          execute(*node.body);
        }
      }
    }
  }

  void Interpreter::visit(ForStmt &node)
  {
    // Evaluate the iterable expression
    Value iterableValue;
    if (node.iterable)
    {
      iterableValue = evaluate(*node.iterable);
    }
    else
    {
      return; // Nothing to iterate over
    }

    // Handle iteration over an array
    if (std::holds_alternative<ArrayValue>(iterableValue))
    {
      const ArrayValue &arr = std::get<ArrayValue>(iterableValue);

      // Create a new environment for the loop scope
      auto loopEnv = std::make_shared<Environment>(environment);

      // Iterate over each element in the array
      for (const auto &element : arr.elements)
      {
        // Define the loop variable in the loop environment
        loopEnv->define(node.variable, element);

        // Execute the loop body with the current loop environment
        if (node.body)
        {
          auto prevEnv = environment;
          environment = loopEnv;
          execute(*node.body);
          environment = prevEnv;
        }
      }
    }
    // Handle iteration over integer ranges (e.g., for (i in 0..10))
    // For now, we'll implement basic range iteration if the iterable is a range
    // Note: This would require implementing range expressions separately
    else
    {
      // For other types, we might want to implement different iteration strategies
      // For now, just return
      return;
    }
  }

  void Interpreter::visit(WhenStmt &node)
  {
    // Evaluate the subject expression
    Value subjectValue;
    if (node.subject)
    {
      subjectValue = evaluate(*node.subject);
    }
    else
    {
      return; // Nothing to match
    }

    // Try each branch pattern to find a match
    bool matched = false;
    for (const auto &branch : node.branches)
    {
      // Evaluate the pattern to compare with the subject
      if (branch.first) // pattern
      {
        Value patternValue = evaluate(*branch.first);

        // Simple equality comparison for now
        if (subjectValue == patternValue)
        {
          // Execute the corresponding statement for this branch
          if (branch.second) // statement
          {
            execute(*branch.second);
          }
          matched = true;
          break; // Exit after first match
        }
      }
    }

    // If no branch matched and there's an else branch, execute it
    if (!matched && node.elseBranch.has_value() && node.elseBranch.value())
    {
      execute(*node.elseBranch.value());
    }
  }

  void Interpreter::visit(ExpressionStmt &node)
  {
    // Evaluate the expression but ignore the result (like in most languages)
    // Expression statements are typically used for side effects
    if (node.expression)
      evaluate(*node.expression);
  }

  Value Interpreter::evaluate(Expression &expr)
  {
    // Check for potential null expression (this shouldn't happen in normal cases, but as a safeguard)
    // Increment evaluation depth and check for recursion limit
    evaluationDepth++;
    if (evaluationDepth > MAX_EVALUATION_DEPTH)
    {
      std::cout << "ERROR: Maximum evaluation depth exceeded, possible infinite recursion" << std::endl;
      evaluationDepth--; // Decrement before returning
      return std::string("recursion_limit_exceeded");
    }

    // Use the visitor pattern to dispatch to the appropriate visit method
    struct EvalVisitor : public AstVisitor
    {
      Value result;
      Interpreter *interpreter;

      EvalVisitor(Interpreter *interp) : interpreter(interp) {}

      void visit(LiteralExpr &node) override
      {
        // Return the literal value directly
        // Convert the old variant type to the new Value type
        if (std::holds_alternative<int>(node.value))
        {
          result = Value(std::get<int>(node.value));
        }
        else if (std::holds_alternative<double>(node.value))
        {
          result = Value(std::get<double>(node.value));
        }
        else if (std::holds_alternative<bool>(node.value))
        {
          result = Value(std::get<bool>(node.value));
        }
        else if (std::holds_alternative<std::string>(node.value))
        {
          result = Value(std::get<std::string>(node.value));
        }
        else
        {
          result = std::string("undefined");
        }
      }

      void visit(IdentifierExpr &node) override
      {
        // Special handling for "args" variable
        if (node.name == "args")
        {
          // Create an array-like representation of command-line arguments
          std::vector<Value> argsVector;
          for (const auto &arg : interpreter->commandLineArgs)
          {
            argsVector.push_back(arg);
          }
          result = Value(ArrayValue(std::move(argsVector)));
        }
        // Look up the value in the environment
        else
        {
          try
          {
            result = interpreter->environment->get(node.name); // This should already return Value type
          }
          catch (const std::exception &e)
          {
            // Return a default value if variable not found
            result = Value(std::string("undefined"));
          }
        }
      }

      void visit(BinaryExpr &node) override
      {
        // Handle assignment specially - don't evaluate left operand as a value
        if (node.op == TokenType::ASSIGN)
        {
          if (auto *identifier = dynamic_cast<IdentifierExpr *>(node.left.get()))
          {
            // Evaluate the right-hand side
            auto rightVal = node.right ? interpreter->evaluate(*node.right) : Value(std::string("null"));

            // Try to assign to the variable in the current environment first
            try
            {
              interpreter->environment->assign(identifier->name, rightVal);
              result = rightVal; // Assignment returns the assigned value
            }
            catch (const std::exception &e)
            {
              // If assignment fails in current environment, try the function environment
              if (interpreter->functionEnvironment && interpreter->functionEnvironment != interpreter->environment)
              {
                try
                {
                  interpreter->functionEnvironment->assign(identifier->name, rightVal);
                  result = rightVal; // Assignment returns the assigned value
                }
                catch (const std::exception &e2)
                {
                  // If assignment fails everywhere, define in the current environment
                  interpreter->environment->define(identifier->name, rightVal);
                  result = rightVal; // Assignment returns the assigned value
                }
              }
              else
              {
                // If no function environment or it's the same as current, define in current
                interpreter->environment->define(identifier->name, rightVal);
                result = rightVal; // Assignment returns the assigned value
              }
            }
            return; // Early return after handling assignment
          }
          else
          {
            // Assignment to non-identifier is invalid
            result = 0;
            return;
          }
        }

        // For non-assignment operations, evaluate both operands and perform operation
        auto leftVal = node.left ? interpreter->evaluate(*node.left) : Value(std::string("null"));
        auto rightVal = node.right ? interpreter->evaluate(*node.right) : Value(std::string("null"));

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
            result = Value(leftStr + rightStr);
          }
          else if (std::holds_alternative<int>(leftVal) &&
                   std::holds_alternative<int>(rightVal))
          {
            result = Value(std::get<int>(leftVal) + std::get<int>(rightVal));
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
            result = Value(left + right);
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
            result = Value(std::get<int>(leftVal) - std::get<int>(rightVal));
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
            result = Value(left - right);
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
            result = Value(std::get<int>(leftVal) * std::get<int>(rightVal));
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
            result = Value(left * right);
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
            result = Value(std::get<int>(rightVal) != 0
                               ? std::get<int>(leftVal) / std::get<int>(rightVal)
                               : 0);
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
            result = Value(right != 0 ? left / right : 0.0);
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
            result = Value(std::get<int>(rightVal) != 0
                               ? std::get<int>(leftVal) % std::get<int>(rightVal)
                               : 0);
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
            result = Value(std::get<int>(leftVal) == std::get<int>(rightVal));
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
            result = Value(left == right);
          }
          else if (std::holds_alternative<std::string>(leftVal) &&
                   std::holds_alternative<std::string>(rightVal))
          {
            result = Value(
                std::get<std::string>(leftVal) == std::get<std::string>(rightVal));
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
        auto operandVal = node.operand ? interpreter->evaluate(*node.operand) : Value(std::string("null"));

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
        std::vector<Value> args;
        for (auto &arg : node.arguments)
        {
          if (arg)
          {
            args.push_back(interpreter->evaluate(*arg));
          }
          else
          {
            args.push_back(Value(std::string("undefined")));
          }
        }

        // Check if it's a method call on an object (callee is MemberAccessExpr)
        if (auto *memberAccess = dynamic_cast<MemberAccessExpr *>(node.callee.get()))
        {
          // Check if the member access has a valid object
          if (!memberAccess->object)
          {
            std::cout << "ERROR: Member access expression has null object" << std::endl;
            result = std::string("null_object_error");
            return;
          }

          // Check if it's the special case of args.size() or args.contentToString()
          std::string objName = "";
          bool isArgsObject = false;
          auto *ident = dynamic_cast<IdentifierExpr *>(memberAccess->object.get());
          if (ident)
          {
            objName = ident->name;
            isArgsObject = (objName == "args");
          }

          if (isArgsObject)
          {
            std::string methodName = memberAccess->property;

            if (objName == "args" && methodName == "size" && args.empty())
            {
              // Handle args.size() method call
              result = static_cast<int>(interpreter->commandLineArgs.size());
              return;
            }
            else if (objName == "args" && methodName == "contentToString" && args.empty())
            {
              // Handle args.contentToString() method call
              std::string content = "[";
              for (size_t i = 0; i < interpreter->commandLineArgs.size(); ++i)
              {
                content += interpreter->commandLineArgs[i];
                if (i < interpreter->commandLineArgs.size() - 1)
                {
                  content += ", ";
                }
              }
              content += "]";
              result = content;
              return;
            }
            else
            {
              // Unrecognized method on args, return default
              result = std::string("");
              return;
            }
          }
          else
          {
            // Check if the object of the member access is a call expression or member access (chained method call)
            // This handles cases like: expr.method().otherMethod() or expr.method1().method2()
            if ((dynamic_cast<CallExpr *>(memberAccess->object.get()) ||
                 dynamic_cast<MemberAccessExpr *>(memberAccess->object.get())))
            {
              // This is a chained method call like: expr.method().otherMethod()
              // First evaluate the object (which could be a call or another member access) to get the base value
              Value baseValue = memberAccess->object ? interpreter->evaluate(*memberAccess->object) : Value(std::string("null"));

              // Now handle method calls on the result
              std::string methodName = memberAccess->property;

              if (methodName == "toString" && args.empty())
              {
                result = interpreter->valueToString(baseValue);
                return;
              }
              else if (methodName == "substring" && args.size() == 1 && std::holds_alternative<int>(args[0]))
              {
                // Handle substring(start) method call
                if (std::holds_alternative<std::string>(baseValue))
                {
                  std::string str = std::get<std::string>(baseValue);
                  int start = std::get<int>(args[0]);
                  if (start >= 0 && static_cast<size_t>(start) <= str.length())
                  {
                    result = str.substr(static_cast<size_t>(start));
                  }
                  else
                  {
                    result = std::string(""); // Return empty string for invalid index
                  }
                }
                else
                {
                  result = std::string(""); // Return empty string for non-string objects
                }
                return;
              }
              else if (methodName == "substring" && args.size() == 2 &&
                       std::holds_alternative<int>(args[0]) && std::holds_alternative<int>(args[1]))
              {
                // Handle substring(start, end) method call
                if (std::holds_alternative<std::string>(baseValue))
                {
                  std::string str = std::get<std::string>(baseValue);
                  int start = std::get<int>(args[0]);
                  int end = std::get<int>(args[1]);
                  if (start >= 0 && static_cast<size_t>(end) <= str.length() && start <= end)
                  {
                    result = str.substr(static_cast<size_t>(start), static_cast<size_t>(end - start));
                  }
                  else
                  {
                    result = std::string(""); // Return empty string for invalid range
                  }
                }
                else
                {
                  result = std::string(""); // Return empty string for non-string objects
                }
                return;
              }
              else if (methodName == "indexOf" && args.size() == 1 && std::holds_alternative<std::string>(args[0]))
              {
                // Handle indexOf(substring) method call
                if (std::holds_alternative<std::string>(baseValue))
                {
                  std::string str = std::get<std::string>(baseValue);
                  std::string substr = std::get<std::string>(args[0]);
                  size_t pos = str.find(substr);
                  result = static_cast<int>(pos != std::string::npos ? static_cast<int>(pos) : -1);
                }
                else
                {
                  result = -1; // Return -1 for non-string objects
                }
                return;
              }
              else if (methodName == "startsWith" && args.size() == 1 && std::holds_alternative<std::string>(args[0]))
              {
                // Handle startsWith(prefix) method call
                if (std::holds_alternative<std::string>(baseValue))
                {
                  std::string str = std::get<std::string>(baseValue);
                  std::string prefix = std::get<std::string>(args[0]);
                  result = static_cast<bool>(str.substr(0, prefix.length()) == prefix);
                }
                else
                {
                  result = false; // Return false for non-string objects
                }
                return;
              }
              else if (methodName == "endsWith" && args.size() == 1 && std::holds_alternative<std::string>(args[0]))
              {
                // Handle endsWith(suffix) method call
                if (std::holds_alternative<std::string>(baseValue))
                {
                  std::string str = std::get<std::string>(baseValue);
                  std::string suffix = std::get<std::string>(args[0]);
                  if (suffix.length() <= str.length())
                  {
                    result = static_cast<bool>(str.substr(str.length() - suffix.length()) == suffix);
                  }
                  else
                  {
                    result = false;
                  }
                }
                else
                {
                  result = false; // Return false for non-string objects
                }
                return;
              }
              else if (methodName == "toUpperCase" && args.empty())
              {
                // Handle toUpperCase() method call
                if (std::holds_alternative<std::string>(baseValue))
                {
                  std::string str = std::get<std::string>(baseValue);
                  std::string upperStr = str;
                  for (char &c : upperStr)
                  {
                    c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
                  }
                  result = upperStr;
                }
                else
                {
                  result = std::string(""); // Return empty string for non-string objects
                }
                return;
              }
              else if (methodName == "toLowerCase" && args.empty())
              {
                // Handle toLowerCase() method call
                if (std::holds_alternative<std::string>(baseValue))
                {
                  std::string str = std::get<std::string>(baseValue);
                  std::string lowerStr = str;
                  for (char &c : lowerStr)
                  {
                    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                  }
                  result = lowerStr;
                }
                else
                {
                  result = std::string(""); // Return empty string for non-string objects
                }
                return;
              }
              else if (methodName == "trim" && args.empty())
              {
                // Handle trim() method call
                if (std::holds_alternative<std::string>(baseValue))
                {
                  std::string str = std::get<std::string>(baseValue);
                  // Left trim
                  size_t start = str.find_first_not_of(" \t\n\r\f\v");
                  if (start == std::string::npos)
                  {
                    result = std::string(""); // String is all whitespace
                  }
                  else
                  {
                    // Right trim
                    size_t end = str.find_last_not_of(" \t\n\r\f\v");
                    result = str.substr(start, end - start + 1);
                  }
                }
                else
                {
                  result = std::string(""); // Return empty string for non-string objects
                }
                return;
              }
              else if (methodName == "split" && args.size() == 1 && std::holds_alternative<std::string>(args[0]))
              {
                // Handle split(delimiter) method call
                if (std::holds_alternative<std::string>(baseValue))
                {
                  std::string str = std::get<std::string>(baseValue);
                  std::string delimiter = std::get<std::string>(args[0]);

                  std::vector<std::string> parts;
                  size_t start = 0;
                  size_t end = str.find(delimiter);

                  while (end != std::string::npos)
                  {
                    parts.push_back(str.substr(start, end - start));
                    start = end + delimiter.length();
                    end = str.find(delimiter, start);
                  }

                  parts.push_back(str.substr(start));

                  // For now, return a string representation of the parts
                  std::string resultStr = "[";
                  for (size_t i = 0; i < parts.size(); ++i)
                  {
                    resultStr += parts[i];
                    if (i < parts.size() - 1)
                    {
                      resultStr += ", ";
                    }
                  }
                  resultStr += "]";
                  result = resultStr;
                }
                else
                {
                  result = std::string(""); // Return empty string for non-string objects
                }
                return;
              }
              else
              {
                // Unrecognized method on chained call, return default
                result = std::string("");
                return;
              }
            }
            else
            {
              // Handle general method calls on expressions (variables, literals, etc.) - direct calls
              std::string methodName = memberAccess->property;

              // Get the object value by evaluating the object expression
              // Safety check for null object
              if (!memberAccess->object)
              {
                std::cout << "ERROR: Direct method call has null object" << std::endl;
                result = std::string("null_object_error");
                return;
              }
              Value objValue = interpreter->evaluate(*memberAccess->object);

              if (methodName == "toString" && args.empty())
              {
                // Handle toString() method for all types
                result = interpreter->valueToString(objValue);
                return;
              }
              else if (methodName == "substring" && args.size() == 1 && std::holds_alternative<int>(args[0]))
              {
                // Handle substring(start) method call
                if (std::holds_alternative<std::string>(objValue))
                {
                  std::string str = std::get<std::string>(objValue);
                  int start = std::get<int>(args[0]);
                  if (start >= 0 && static_cast<size_t>(start) <= str.length())
                  {
                    result = str.substr(static_cast<size_t>(start));
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
                  if (start >= 0 && static_cast<size_t>(end) <= str.length() && start <= end)
                  {
                    result = str.substr(static_cast<size_t>(start), static_cast<size_t>(end - start));
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
                  result = static_cast<int>(pos != std::string::npos ? static_cast<int>(pos) : -1);
                }
                else
                {
                  result = -1; // Return -1 for non-string objects
                }
                return;
              }
              else if (methodName == "startsWith" && args.size() == 1 && std::holds_alternative<std::string>(args[0]))
              {
                // Handle startsWith(prefix) method call
                if (std::holds_alternative<std::string>(objValue))
                {
                  std::string str = std::get<std::string>(objValue);
                  std::string prefix = std::get<std::string>(args[0]);
                  result = static_cast<bool>(str.substr(0, prefix.length()) == prefix);
                }
                else
                {
                  result = false; // Return false for non-string objects
                }
                return;
              }
              else if (methodName == "endsWith" && args.size() == 1 && std::holds_alternative<std::string>(args[0]))
              {
                // Handle endsWith(suffix) method call
                if (std::holds_alternative<std::string>(objValue))
                {
                  std::string str = std::get<std::string>(objValue);
                  std::string suffix = std::get<std::string>(args[0]);
                  if (suffix.length() <= str.length())
                  {
                    result = static_cast<bool>(str.substr(str.length() - suffix.length()) == suffix);
                  }
                  else
                  {
                    result = false;
                  }
                }
                return;
              }
              else if (methodName == "toUpperCase" && args.empty())
              {
                // Handle toUpperCase() method call
                if (std::holds_alternative<std::string>(objValue))
                {
                  std::string str = std::get<std::string>(objValue);
                  std::string upperStr = str;
                  for (char &c : upperStr)
                  {
                    c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
                  }
                  result = upperStr;
                }
                else
                {
                  result = std::string(""); // Return empty string for non-string objects
                }
                return;
              }
              else if (methodName == "toLowerCase" && args.empty())
              {
                // Handle toLowerCase() method call
                if (std::holds_alternative<std::string>(objValue))
                {
                  std::string str = std::get<std::string>(objValue);
                  std::string lowerStr = str;
                  for (char &c : lowerStr)
                  {
                    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                  }
                  result = lowerStr;
                }
                else
                {
                  result = std::string(""); // Return empty string for non-string objects
                }
                return;
              }
              else if (methodName == "trim" && args.empty())
              {
                // Handle trim() method call
                if (std::holds_alternative<std::string>(objValue))
                {
                  std::string str = std::get<std::string>(objValue);
                  // Left trim
                  size_t start = str.find_first_not_of(" \t\n\r\f\v");
                  if (start == std::string::npos)
                  {
                    result = std::string(""); // String is all whitespace
                  }
                  else
                  {
                    // Right trim
                    size_t end = str.find_last_not_of(" \t\n\r\f\v");
                    result = str.substr(start, end - start + 1);
                  }
                }
                else
                {
                  result = std::string(""); // Return empty string for non-string objects
                }
                return;
              }
              else if (methodName == "split" && args.size() == 1 && std::holds_alternative<std::string>(args[0]))
              {
                // Handle split(delimiter) method call
                if (std::holds_alternative<std::string>(objValue))
                {
                  std::string str = std::get<std::string>(objValue);
                  std::string delimiter = std::get<std::string>(args[0]);

                  std::vector<std::string> parts;
                  size_t start = 0;
                  size_t end = str.find(delimiter);

                  while (end != std::string::npos)
                  {
                    parts.push_back(str.substr(start, end - start));
                    start = end + delimiter.length();
                    end = str.find(delimiter, start);
                  }

                  parts.push_back(str.substr(start));

                  // For now, return a string representation of the parts
                  std::string resultStr = "[";
                  for (size_t i = 0; i < parts.size(); ++i)
                  {
                    resultStr += parts[i];
                    if (i < parts.size() - 1)
                    {
                      resultStr += ", ";
                    }
                  }
                  resultStr += "]";
                  result = resultStr;
                }
                else
                {
                  result = std::string(""); // Return empty string for non-string objects
                }
                return;
              }
              else
              {
                // Unrecognized method on direct call, return default
                result = std::string("");
                return;
              }
            }
          }
        }
        // If we reach here and the callee is not a MemberAccessExpr, it means we need to handle
        // chained method calls differently. Actually, the structure is correct:
        // When we have expr.method().otherMethod(), the outer call is otherMethod() on the result
        // of expr.method(), so the callee of the outer call is the MemberAccessExpr for .otherMethod()
        // The original structure was correct but had a syntax error. We should check for non-MemberAccess
        // call expressions after the main MemberAccess handling.

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
          else if (ident->name == "printf" || ident->name == "format")
          {
            // Handle printf-style formatting
            if (!args.empty())
            {
              // First argument is the format string
              std::string formatStr = interpreter->valueToString(args[0]);

              // For now, implement basic string replacement
              std::string resultStr = formatStr;

              // Replace placeholders like %s, %d, %f with actual values
              size_t argIndex = 1;
              for (size_t i = 0; i < resultStr.length() && argIndex < args.size(); ++i)
              {
                if (resultStr[i] == '%' && i + 1 < resultStr.length())
                {
                  char nextChar = resultStr[i + 1];
                  if (nextChar == 's' || nextChar == 'd' || nextChar == 'f' || nextChar == 'i')
                  {
                    std::string replacement = interpreter->valueToString(args[argIndex]);
                    resultStr.replace(i, 2, replacement);
                    ++argIndex;
                    i += replacement.length() - 1; // Adjust position after replacement
                  }
                }
              }

              result = resultStr;
            }
            else
            {
              result = std::string("");
            }
          }
          else if (ident->name == "toInt")
          {
            // Convert string to integer
            if (!args.empty())
            {
              std::string str = interpreter->valueToString(args[0]);
              try
              {
                int val = std::stoi(str);
                result = val;
              }
              catch (...)
              {
                result = 0; // Return 0 on conversion error
              }
            }
            else
            {
              result = 0;
            }
          }
          else if (ident->name == "toDouble")
          {
            // Convert string to double
            if (!args.empty())
            {
              std::string str = interpreter->valueToString(args[0]);
              try
              {
                double val = std::stod(str);
                result = val;
              }
              catch (...)
              {
                result = 0.0; // Return 0.0 on conversion error
              }
            }
            else
            {
              result = 0.0;
            }
          }
          else if (ident->name == "arrayOf")
          {
            // Create an array from the arguments
            result = ArrayValue(std::vector<Value>(args.begin(), args.end()));
          }
          else if (ident->name == "intArrayOf")
          {
            // Create an integer array from the arguments (convert all to integers)
            std::vector<Value> intElements;
            for (const auto &arg : args)
            {
              if (std::holds_alternative<int>(arg))
              {
                intElements.push_back(arg);
              }
              else if (std::holds_alternative<double>(arg))
              {
                intElements.push_back(static_cast<int>(std::get<double>(arg)));
              }
              else if (std::holds_alternative<std::string>(arg))
              {
                try
                {
                  int val = std::stoi(std::get<std::string>(arg));
                  intElements.push_back(val);
                }
                catch (...)
                {
                  intElements.push_back(0); // default value
                }
              }
              else
              {
                intElements.push_back(0); // default value
              }
            }
            result = ArrayValue(std::move(intElements));
          }
          else if (ident->name == "doubleArrayOf")
          {
            // Create a double array from the arguments (convert all to doubles)
            std::vector<Value> doubleElements;
            for (const auto &arg : args)
            {
              if (std::holds_alternative<double>(arg))
              {
                doubleElements.push_back(arg);
              }
              else if (std::holds_alternative<int>(arg))
              {
                doubleElements.push_back(static_cast<double>(std::get<int>(arg)));
              }
              else if (std::holds_alternative<std::string>(arg))
              {
                try
                {
                  double val = std::stod(std::get<std::string>(arg));
                  doubleElements.push_back(val);
                }
                catch (...)
                {
                  doubleElements.push_back(0.0); // default value
                }
              }
              else
              {
                doubleElements.push_back(0.0); // default value
              }
            }
            result = ArrayValue(std::move(doubleElements));
          }
          else if (ident->name == "stringArrayOf")
          {
            // Create a string array from the arguments (convert all to strings)
            std::vector<Value> stringElements;
            for (const auto &arg : args)
            {
              stringElements.push_back(interpreter->valueToString(arg));
            }
            result = ArrayValue(std::move(stringElements));
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
        if (!node.object)
        {
          std::cout << "ERROR: Member access expression has null object" << std::endl;
          result = std::string("null_object_error");
          return;
        }
        auto objValue = node.object ? interpreter->evaluate(*node.object) : Value(std::string("null"));

        // Check if the object is an identifier "args" and the property is "size" or "contentToString"
        if (auto *identifier = dynamic_cast<IdentifierExpr *>(node.object.get()))
        {
          if (identifier->name == "args" && node.property == "size")
          {
            // Return the size of command-line arguments
            result = static_cast<int>(interpreter->commandLineArgs.size());
            return;
          }
          else if (identifier->name == "args" && node.property == "contentToString")
          {
            // Return a string representation of all command-line arguments
            std::string content = "[";
            for (size_t i = 0; i < interpreter->commandLineArgs.size(); ++i)
            {
              content += interpreter->commandLineArgs[i];
              if (i < interpreter->commandLineArgs.size() - 1)
              {
                content += ", ";
              }
            }
            content += "]";
            result = content;
            return;
          }
        }

        if (node.property == "length" && std::holds_alternative<std::string>(objValue))
        {
          // Handle .length property for strings
          std::string str = std::get<std::string>(objValue);
          result = static_cast<int>(str.length()); // Return the length as an integer
        }
        else if (node.property == "size" && std::holds_alternative<ArrayValue>(objValue))
        {
          // Handle .size property for arrays
          const ArrayValue &arr = std::get<ArrayValue>(objValue);
          result = static_cast<int>(arr.elements.size());
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
        else if (node.property == "startsWith" && std::holds_alternative<std::string>(objValue))
        {
          // Handle .startsWith property access without arguments
          // For now, return a placeholder - startsWith with arguments is handled in CallExpr
          result = std::string("");
        }
        else if (node.property == "endsWith" && std::holds_alternative<std::string>(objValue))
        {
          // Handle .endsWith property access without arguments
          // For now, return a placeholder - endsWith with arguments is handled in CallExpr
          result = std::string("");
        }
        else if (node.property == "toUpperCase" && std::holds_alternative<std::string>(objValue))
        {
          // Handle .toUpperCase property access without arguments
          // For now, return a placeholder - toUpperCase with arguments is handled in CallExpr
          result = std::string("");
        }
        else if (node.property == "toLowerCase" && std::holds_alternative<std::string>(objValue))
        {
          // Handle .toLowerCase property access without arguments
          // For now, return a placeholder - toLowerCase with arguments is handled in CallExpr
          result = std::string("");
        }
        else if (node.property == "trim" && std::holds_alternative<std::string>(objValue))
        {
          // Handle .trim property access without arguments
          // For now, return a placeholder - trim with arguments is handled in CallExpr
          result = std::string("");
        }
        else if (node.property == "split" && std::holds_alternative<std::string>(objValue))
        {
          // Handle .split property access without arguments
          // For now, return a placeholder - split with arguments is handled in CallExpr
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
        // Handle array access like args[0] or myArray[1]
        if (!node.array || !node.index)
        {
          std::cout << "ERROR: Array access expression has null array or index" << std::endl;
          result = std::string("null_access_error");
          return;
        }
        auto arrayValue = node.array ? interpreter->evaluate(*node.array) : Value(std::string("null"));
        auto indexValue = node.index ? interpreter->evaluate(*node.index) : Value(std::string("null"));

        // Check if we're accessing the args array (legacy handling)
        if (std::holds_alternative<std::string>(arrayValue) &&
            std::get<std::string>(arrayValue) == "args_array" &&
            std::holds_alternative<int>(indexValue))
        {
          int index = std::get<int>(indexValue);
          if (index >= 0 && static_cast<size_t>(index) < interpreter->commandLineArgs.size())
          {
            result = interpreter->commandLineArgs[static_cast<size_t>(index)];
          }
          else
          {
            result = std::string("index_out_of_bounds");
          }
        }
        // Handle access to actual ArrayValue
        else if (std::holds_alternative<ArrayValue>(arrayValue) &&
                 std::holds_alternative<int>(indexValue))
        {
          const ArrayValue &arr = std::get<ArrayValue>(arrayValue);
          int index = std::get<int>(indexValue);

          if (index >= 0 && static_cast<size_t>(index) < arr.elements.size())
          {
            result = arr.elements[static_cast<size_t>(index)];
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

      void visit(ArrayLiteralExpr &node) override
      {
        // Evaluate each element in the array literal
        std::vector<Value> elements;
        for (auto &element : node.elements)
        {
          if (element)
          {
            elements.push_back(interpreter->evaluate(*element));
          }
          else
          {
            elements.push_back(Value(std::string("undefined")));
          }
        }
        // Create an ArrayValue with the evaluated elements
        result = ArrayValue(std::move(elements));
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
      void visit(WhileStmt &node) override
      {
        result = std::string("error: expression visitor called on statement");
        (void)node;
      }
      void visit(ForStmt &node) override
      {
        result = std::string("error: expression visitor called on statement");
        (void)node;
      }
      void visit(WhenStmt &node) override
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
      // Return a default value in case of error
      evaluationDepth--; // Decrement before returning
      return std::string("evaluation_error");
    }
    catch (...)
    {
      std::cout << "Unknown exception in expression evaluation" << std::endl;
      evaluationDepth--; // Decrement before returning
      return std::string("evaluation_error");
    }
    // Decrement evaluation depth before returning
    evaluationDepth--;
    return visitor.result;
  }

  void
  Interpreter::execute(Statement &stmt)
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
        // Handle function calls for side effects (like println)
        interpreter->visit(node);
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
      void visit(ArrayLiteralExpr &node) override
      {
        // Evaluate the array literal expression
        interpreter->evaluate(node);
      }
      void visit(ExpressionStmt &node) override { interpreter->visit(node); }
      void visit(VariableDeclStmt &node) override { interpreter->visit(node); }
      void visit(FunctionDeclStmt &node) override { interpreter->visit(node); }
      void visit(BlockStmt &node) override { interpreter->visit(node); }
      void visit(ReturnStmt &node) override { interpreter->visit(node); }
      void visit(IfStmt &node) override { interpreter->visit(node); }
      void visit(WhileStmt &node) override { interpreter->visit(node); }
      void visit(ForStmt &node) override { interpreter->visit(node); }
      void visit(WhenStmt &node) override { interpreter->visit(node); }
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
    else if (std::holds_alternative<ArrayValue>(value))
    {
      std::string result = "[";
      std::vector<Value> arr = std::get<ArrayValue>(value).elements;
      for (size_t i = 0; i < arr.size(); ++i)
      {
        result += valueToString(arr[i]);
        if (i < arr.size() - 1)
        {
          result += ", ";
        }
      }
      result += "]";
      return result;
    }
    else
    {
      return "undefined";
    }
  }

  Value Interpreter::executeBlock(const std::vector<Statement::Ptr> &statements,
                                  std::shared_ptr<Environment> parentEnv)
  {
    // Create new environment that inherits from parent
    auto blockEnv = std::make_shared<Environment>(parentEnv, true); // Mark as block scope
    auto previousEnv = environment;
    environment = blockEnv;

    Value result;
    for (const auto &stmt : statements)
    {
      execute(*stmt);
    }

    environment = previousEnv; // Restore previous environment
    return result;
  }

} // namespace dotlin