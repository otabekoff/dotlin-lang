#include "dotlin/interpreter.h"
#include "dotlin/parser.h"
#include "dotlin/visitors.h"
// #include <chrono>
// #include <iostream>
// #include <iostream>
#include <stdexcept>

using namespace dotlin;

namespace dotlin {

// Define the static member
std::map<std::string, std::vector<std::shared_ptr<FunctionDef>>>
    Interpreter::functionDefinitions;

std::string Interpreter::valueToString(const Value &value) {
  return dotlin::valueToString(value);
}

std::string Interpreter::typeToString(const std::shared_ptr<Type> &type) {
  return dotlin::typeToString(type);
}

Interpreter::Interpreter()
    : globals(std::make_shared<Environment>()), environment(globals),
      functionEnvironment(nullptr), hasMainFunction(false),
      mainFunctionStmt(nullptr), commandLineArgs({}) {}

void Interpreter::resolve(const Expression *expr, int depth, int index) {
  locals[expr] = {depth, index};
}

std::optional<std::pair<int, int>>
Interpreter::getResolvedLocation(const Expression *expr) {
  auto it = locals.find(expr);
  if (it != locals.end()) {
    return it->second;
  }
  return std::nullopt;
}

// Helper to trace lookups in Evaluator (will be called from Evaluator)
void Interpreter::traceLookup(const std::string &name,
                              std::optional<int> distance) {
  std::cout << "[DEBUG] Looking up '" << name << "' at distance "
            << (distance.has_value() ? std::to_string(*distance) : "global")
            << std::endl;
}

Value Interpreter::interpret(const Program &program) {
  std::vector<std::string> empty_args;
  return interpret(program, empty_args, sourceName);
}

Value Interpreter::interpret(const Program &program,
                             const std::vector<std::string> &args) {
  return interpret(program, args, sourceName);
}

Value Interpreter::interpret(const Program &program,
                             const std::vector<std::string> &args,
                             const std::string &srcName) {
  sourceName = srcName;
  // Store command-line arguments
  commandLineArgs = args;

  // Perform type inference before execution
  performTypeInference(const_cast<Program &>(program));

  // Perform optimizations
  performOptimization(const_cast<Program &>(program));

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
          }
          environment->define(paramName, defaultValue);
        }
      }

      try {
        if (mainDef->body) {
          callStack.push_back("main");
          execute(*mainDef->body);
          callStack.pop_back();
        }
      } catch (DotlinError &e) {
        if (e.stackTrace.empty()) {
          e.setStackTrace(callStack);
        }
        callStack.pop_back();
        environment = previousEnv;
        functionEnvironment = previousFuncEnv;
        throw;
      } catch (const std::runtime_error &e) {
        callStack.pop_back();
        environment = previousEnv;
        functionEnvironment = previousFuncEnv;
        std::string msg = e.what();
        if (msg.substr(0, 7) == "RETURN:") {
          return Value(msg.substr(7)); // Remove "RETURN:" prefix
        }
        throw;
      }
    }
  }

  return Value(std::string("Program executed successfully"));
}

Value Interpreter::executeBlock(
    const std::vector<std::shared_ptr<Statement>> &statements,
    std::shared_ptr<Environment> blockEnvironment) {
  auto previous = environment;
  try {
    environment = blockEnvironment;
    for (const auto &stmt : statements) {
      if (stmt) {
        execute(*stmt);
      }
    }
  } catch (const std::runtime_error &e) {
    environment = previous;
    throw;
  }
  environment = previous;
  return Value(); // Return default Value
}

Value Interpreter::evaluate(Expression &expr) {
  try {
    EvalVisitor visitor(this);
    expr.accept(visitor);
    lastEvaluatedValue = visitor.result;
    return visitor.result;
  } catch (DotlinError &e) {
    e.setSource(sourceName);
    throw;
  }
}

Value Interpreter::evaluate(Expression::Ptr &exprPtr) {
  if (!exprPtr) {
    throw std::runtime_error("Attempted to evaluate NULL expression!");
  }
  return evaluate(*exprPtr);
}

void Interpreter::execute(Statement &stmt) {
  try {
    ExecVisitor visitor(this);
    stmt.accept(visitor);
  } catch (DotlinError &e) {
    e.setSource(sourceName);
    throw;
  }
}

Value Interpreter::executeFunction(const std::string &name, Statement *body,
                                   std::shared_ptr<Environment> funcEnv) {
  if (!body) {
    return Value();
  }

  auto previousEnv = environment;
  environment = funcEnv;
  // Initialize lastEvaluatedValue to a default (0 / Unit)
  lastEvaluatedValue = Value();

  callStack.push_back(name);

  try {
    execute(*body);
  } catch (DotlinError &e) {
    if (e.stackTrace.empty()) {
      e.setStackTrace(callStack);
    }
    callStack.pop_back();
    environment = previousEnv;
    throw;
  } catch (const std::runtime_error &e) {
    callStack.pop_back();
    environment = previousEnv;
    std::string msg = e.what();
    if (msg == "RETURN_SIGNAL") {
      return lastEvaluatedValue;
    }
    throw;
  }

  callStack.pop_back();
  environment = previousEnv;
  return lastEvaluatedValue;
}

std::string getTypeOfValue(const Value &value);
std::string typeToString(const std::shared_ptr<Type> &type);

std::shared_ptr<FunctionDef>
Interpreter::findBestFunctionOverload(const std::string &name,
                                      const std::vector<Value> &args) {
  auto it = Interpreter::functionDefinitions.find(name);
  if (it == Interpreter::functionDefinitions.end()) {
    return nullptr;
  }

  // Look for a matching function (overload resolution)
  for (const auto &func : it->second) {
    if (func->parameters.size() == args.size()) {
      bool typesMatch = true;
      for (size_t i = 0; i < func->parameters.size(); ++i) {
        if (func->parameters[i].typeAnnotation &&
            (*func->parameters[i].typeAnnotation)->kind != TypeKind::UNKNOWN) {
          std::string expected =
              typeToString(*func->parameters[i].typeAnnotation);
          std::string actual = ::dotlin::getTypeOfValue(args[i]);

          auto normalize = [](const std::string &s) {
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

          if (normalize(expected) != actual) {
            typesMatch = false;
            break;
          }
        }
      }

      if (typesMatch) {
        return func;
      }
    }
  }

  return nullptr;
}

// Global interpret function
Value interpret(const Program &program) {
  Interpreter interpreter;
  return interpreter.interpret(program);
}

Value interpret(const Program &program, const std::vector<std::string> &args) {
  Interpreter interpreter;
  return interpreter.interpret(program, args);
}

Value interpret(const Program &program, const std::vector<std::string> &args,
                const std::string &sourceName) {
  Interpreter interpreter;
  interpreter.setSourceName(sourceName);
  return interpreter.interpret(program, args);
}

// Perform type inference on a program
void Interpreter::performTypeInference(Program &program) {
  // Create a type environment for static analysis
  auto typeEnv = std::make_shared<TypeEnvironment>();

  // Register built-in functions in type environment
  typeEnv->define("println", std::make_shared<Type>(TypeKind::VOID));
  typeEnv->define("print", std::make_shared<Type>(TypeKind::VOID));
  typeEnv->define("readln", std::make_shared<Type>(TypeKind::STRING));
  typeEnv->define("readLine", std::make_shared<Type>(TypeKind::STRING));
  typeEnv->define("sqrt", std::make_shared<Type>(TypeKind::DOUBLE));
  typeEnv->define("abs", std::make_shared<Type>(TypeKind::DOUBLE));
  typeEnv->define("sin", std::make_shared<Type>(TypeKind::DOUBLE));
  typeEnv->define("cos", std::make_shared<Type>(TypeKind::DOUBLE));
  typeEnv->define("tan", std::make_shared<Type>(TypeKind::DOUBLE));
  typeEnv->define("now", std::make_shared<Type>(TypeKind::LONG));
  typeEnv->define("currentTimeMillis", std::make_shared<Type>(TypeKind::LONG));

  // Create a type checker instance
  TypeChecker typeChecker(typeEnv, environment);

  // Perform type inference for each statement in program
  for (auto &stmt : program.statements) {
    typeChecker.checkStatement(*stmt);
  }
}

void Interpreter::performOptimization(Program &program) {
  // First perform constant folding
  ConstantFolderVisitor folder;
  for (auto &stmt : program.statements) {
    stmt = folder.fold(stmt);
  }

  // Perform variable resolution (Resolver)
  ResolverVisitor resolver(this);
  resolver.resolve(program.statements);

  // Then perform dead code elimination
  DeadCodeEliminationVisitor dce;
  for (auto &stmt : program.statements) {
    stmt = dce.eliminate(stmt);
  }
}
} // namespace dotlin