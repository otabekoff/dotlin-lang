#include "dotlin/interpreter.h"
#include "dotlin/parser.h"
#include "dotlin/visitors.h"
// #include <chrono>
// #include <iostream>
#include <iostream>
#include <stdexcept>

using namespace dotlin;

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
          }
          environment->define(paramName, defaultValue);
        }
      }

      try {
        auto mainFunc =
            dynamic_cast<FunctionDeclStmt *>(mainFunctionStmt.get());
        if (mainFunc) {
          Value result = executeFunction(mainFunc->body.get(), environment);
          environment = previousEnv;
          functionEnvironment = previousFuncEnv;
          return result;
        }
      } catch (const std::runtime_error &e) {
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
      execute(*stmt);
    }
  } catch (const std::runtime_error &e) {
    environment = previous;
    throw;
  }
  environment = previous;
  return Value(); // Return default Value
}

Value Interpreter::evaluate(Expression &expr) {
  EvalVisitor visitor(this);
  expr.accept(visitor);
  return visitor.result;
}

void Interpreter::execute(Statement &stmt) {
  ExecVisitor visitor(this);
  stmt.accept(visitor);
}

Value Interpreter::executeFunction(Statement *body,
                                   std::shared_ptr<Environment> funcEnv) {
  if (!body) {
    return Value();
  }

  auto previousEnv = environment;
  environment = funcEnv;

  try {
    execute(*body);
  } catch (const std::runtime_error &e) {
    environment = previousEnv;
    std::string msg = e.what();
    if (msg.substr(0, 7) == "RETURN:") {
      return Value(msg.substr(7)); // Remove "RETURN:" prefix
    }
    throw;
  }

  environment = previousEnv;
  return Value();
}

std::shared_ptr<FunctionDef>
Interpreter::findBestFunctionOverload(const std::string &name,
                                      const std::vector<Value> & /*args*/) {
  // For now, just return the first function found with the given name
  // In a full implementation, we'd implement proper overload resolution
  if (name == "main" && mainFunctionStmt) {
    auto mainFunc = dynamic_cast<FunctionDeclStmt *>(mainFunctionStmt.get());
    if (mainFunc) {
      return std::make_shared<FunctionDef>(mainFunc->name, mainFunc->parameters,
                                           mainFunc->body);
    }
  }
  return nullptr;
}

// Type checking methods
std::shared_ptr<Type> TypeChecker::checkExpression(Expression &expr) {
  TypeCheckVisitor visitor(this);
  expr.accept(visitor);
  return visitor.result;
}

void TypeChecker::checkStatement(Statement &stmt) {
  StmtTypeCheckVisitor visitor(this);
  stmt.accept(visitor);
}

// Helper method to convert value to string
std::string Interpreter::valueToString(const Value &value) {
  if (std::holds_alternative<int>(value)) {
    return std::to_string(std::get<int>(value));
  } else if (std::holds_alternative<double>(value)) {
    return std::to_string(std::get<double>(value));
  } else if (std::holds_alternative<bool>(value)) {
    return std::get<bool>(value) ? "true" : "false";
  } else if (std::holds_alternative<std::string>(value)) {
    return std::get<std::string>(value);
  } else if (std::holds_alternative<ArrayValue>(value)) {
    const auto &array = std::get<ArrayValue>(value);
    std::string result = "[";
    for (size_t i = 0; i < array.elements.size(); ++i) {
      if (i > 0)
        result += ", ";
      result += valueToString(array.elements[i]);
    }
    result += "]";
    return result;
  } else if (std::holds_alternative<std::shared_ptr<LambdaValue>>(value)) {
    return "<lambda>";
  } else if (std::holds_alternative<std::shared_ptr<ClassInstance>>(value)) {
    const auto &instance = std::get<std::shared_ptr<ClassInstance>>(value);
    return "<instance of " + instance->className + ">";
  } else if (std::holds_alternative<std::shared_ptr<ClassDefinition>>(value)) {
    const auto &classDef = std::get<std::shared_ptr<ClassDefinition>>(value);
    return "<class " + classDef->name + ">";
  }
  return "<unknown>";
}

// Helper method to convert type to string
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
    if (type->elementType) {
      return "Array<" + typeToString(type->elementType) + ">";
    }
    return "Array";
  case TypeKind::VOID:
    return "Unit";
  case TypeKind::UNKNOWN:
    return "Unknown";
  case TypeKind::ANY:
    return "Any";
  default:
    return "unknown";
  }
}

void Interpreter::performTypeInferenceOnExpression(Expression &expr,
                                                   TypeChecker &typeChecker) {
  // Visit child expressions recursively
  if (auto *binaryExpr = dynamic_cast<BinaryExpr *>(&expr)) {
    if (binaryExpr->left) {
      performTypeInferenceOnExpression(*binaryExpr->left, typeChecker);
    }
    if (binaryExpr->right) {
      performTypeInferenceOnExpression(*binaryExpr->right, typeChecker);
    }
  } else if (auto *unaryExpr = dynamic_cast<UnaryExpr *>(&expr)) {
    if (unaryExpr->operand) {
      performTypeInferenceOnExpression(*unaryExpr->operand, typeChecker);
    }
  } else if (auto *callExpr = dynamic_cast<CallExpr *>(&expr)) {
    for (auto &arg : callExpr->arguments) {
      if (arg) {
        performTypeInferenceOnExpression(*arg, typeChecker);
      }
    }
  } else if (auto *memberAccess = dynamic_cast<MemberAccessExpr *>(&expr)) {
    if (memberAccess->object) {
      performTypeInferenceOnExpression(*memberAccess->object, typeChecker);
    }
  } else if (auto *arrayAccess = dynamic_cast<ArrayAccessExpr *>(&expr)) {
    if (arrayAccess->array) {
      performTypeInferenceOnExpression(*arrayAccess->array, typeChecker);
    }
    if (arrayAccess->index) {
      performTypeInferenceOnExpression(*arrayAccess->index, typeChecker);
    }
  } else if (auto *arrayLiteral = dynamic_cast<ArrayLiteralExpr *>(&expr)) {
    for (auto &element : arrayLiteral->elements) {
      if (element) {
        performTypeInferenceOnExpression(*element, typeChecker);
      }
    }
  }
  // For identifier expressions, we can get the type from the environment
  // but don't need to recurse further
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

// Perform type inference on a program
void Interpreter::performTypeInference(Program &program) {
  // Create a type checker instance
  TypeChecker typeChecker(environment);

  // Perform type inference for each statement in program
  for (auto &stmt : program.statements) {
    performTypeInferenceOnStatement(*stmt, typeChecker);
  }
}

// Perform type inference on a statement
void Interpreter::performTypeInferenceOnStatement(Statement &stmt,
                                                  TypeChecker &typeChecker) {
  // For variable declarations, infer type from initializer if no
  // explicit type annotation is provided
  if (auto *varDecl = dynamic_cast<VariableDeclStmt *>(&stmt)) {
    if (!varDecl->typeAnnotation.has_value() &&
        varDecl->initializer.has_value()) {
      // Infer the type from the initializer expression
      auto inferredType =
          typeChecker.checkExpression(*(varDecl->initializer.value()));

      // Store the inferred type in the variable declaration
      varDecl->typeAnnotation = inferredType;

      std::cout << "Type inferred for variable '" << varDecl->name
                << "': " << typeToString(inferredType) << std::endl;
    }
  } else if (auto *blockStmt = dynamic_cast<BlockStmt *>(&stmt)) {
    // Recursively perform type inference on statements in block
    for (auto &innerStmt : blockStmt->statements) {
      performTypeInferenceOnStatement(*innerStmt, typeChecker);
    }
  } else if (auto *ifStmt = dynamic_cast<IfStmt *>(&stmt)) {
    // Perform type inference on condition and branches
    if (ifStmt->condition) {
      performTypeInferenceOnExpression(*ifStmt->condition, typeChecker);
    }
    if (ifStmt->thenBranch) {
      performTypeInferenceOnStatement(*ifStmt->thenBranch, typeChecker);
    }
    if (ifStmt->elseBranch.has_value() && ifStmt->elseBranch.value()) {
      performTypeInferenceOnStatement(*(ifStmt->elseBranch.value()),
                                      typeChecker);
    }
  } else if (auto *whileStmt = dynamic_cast<WhileStmt *>(&stmt)) {
    // Perform type inference on condition and body
    if (whileStmt->condition) {
      performTypeInferenceOnExpression(*whileStmt->condition, typeChecker);
    }
    if (whileStmt->body) {
      performTypeInferenceOnStatement(*whileStmt->body, typeChecker);
    }
  } else if (auto *forStmt = dynamic_cast<ForStmt *>(&stmt)) {
    // Perform type inference on iterable and body
    if (forStmt->iterable) {
      performTypeInferenceOnExpression(*forStmt->iterable, typeChecker);
    }
    if (forStmt->body) {
      performTypeInferenceOnStatement(*forStmt->body, typeChecker);
    }
  }
  // Add more statement types as needed
}

} // namespace dotlin
