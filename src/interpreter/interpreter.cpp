#include "dotlin/interpreter.h"
// #include <iostream>
#include <stdexcept>

namespace dotlin {

Interpreter::Interpreter()
    : globals(std::make_shared<Environment>()), environment(globals),
      functionEnvironment(nullptr), hasMainFunction(false),
      mainFunctionStmt(nullptr), commandLineArgs({}) {}

Value Interpreter::interpret(const Program &program) {
  std::vector<std::string> empty_args;
  return interpret(program, empty_args);
}

Value Interpreter::interpret(const Program &program,
                             const std::vector<std::string> &args) {
  // Store command-line arguments
  commandLineArgs = args;

  // First, execute all statements to register functions and declare variables
  for (const auto &stmt : program.statements) {
    execute(*stmt);
  }

  if (hasMainFunction) {
    std::vector<Value> mainArgs;
    for (const auto &arg : commandLineArgs) {
      mainArgs.push_back(arg);
    }

    auto mainDef = findBestFunctionOverload("main", mainArgs);
    if (mainDef) {
      auto previousEnv = environment;
      auto previousFuncEnv = functionEnvironment;
      environment = std::make_shared<Environment>(globals, false);
      functionEnvironment = environment;

      for (size_t i = 0; i < mainDef->parameters.size(); ++i) {
        std::string paramName = mainDef->parameters[i].name;
        if (i < commandLineArgs.size()) {
          environment->define(paramName, commandLineArgs[i]);
        } else {
          Value defaultValue;
          if (mainDef->parameters[i].typeAnnotation.has_value() &&
              mainDef->parameters[i].typeAnnotation.value()) {
            auto type = mainDef->parameters[i].typeAnnotation.value();
            switch (type->kind) {
            case TypeKind::INT:
              defaultValue = 0;
              break;
            case TypeKind::DOUBLE:
              defaultValue = 0.0;
              break;
            case TypeKind::BOOL:
              defaultValue = false;
              break;
            case TypeKind::STRING:
              defaultValue = std::string("");
              break;
            default:
              defaultValue = std::string("");
              break;
            }
          } else {
            defaultValue = std::string("");
          }
          environment->define(paramName, defaultValue);
        }
      }

      try {
        if (mainDef->body) {
          execute(*mainDef->body);
        }
      } catch (const std::runtime_error &e) {
        std::string msg = e.what();
        if (msg.substr(0, 7) != "RETURN:") {
          throw;
        }
        // Handle return value from main function
        return Value(msg.substr(7)); // Remove "RETURN:" prefix
      }

      environment = previousEnv;
      functionEnvironment = previousFuncEnv;
    }
  }

  // For now, return a dummy value
  return Value(std::string("Program executed successfully"));
}

Value Interpreter::executeBlock(
    const std::vector<std::shared_ptr<Statement>> &statements,
    std::shared_ptr<Environment> blockEnvironment) {
  auto previous = environment;
  try {
    environment = blockEnvironment;
    for (const auto &statement : statements) {
      if (statement) {
        execute(*statement);
      }
    }
  } catch (...) {
    environment = previous;
    throw;
  }
  environment = previous;
  return Value(); // Return default Value
}

Value interpret(const Program &program) {
  Interpreter interpreter;
  return interpreter.interpret(program);
}

} // namespace dotlin
