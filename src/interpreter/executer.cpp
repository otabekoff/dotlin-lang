#include "dotlin/interpreter.h"
#include "dotlin/parser.h"
#include "dotlin/visitors.h"
#include <iostream>
#include <stdexcept>

// Forward declaration of valueToString
namespace dotlin {
std::string valueToString(const Value &value);
}

using namespace dotlin;

// Statement execution visitor implementations
void ExecVisitor::visit(ExpressionStmt &node) {
  // Execute the expression for its side effects and store for functional return
  interpreter->lastEvaluatedValue = interpreter->evaluate(*node.expression);
}

void ExecVisitor::visit(VariableDeclStmt &node) {
  Value value;
  if (node.initializer) {
    if (!node.initializer.value()) {
      std::cout << "CRITICAL: Initializer is NULL for " << node.name
                << std::endl;
    }
    value = interpreter->evaluate(*node.initializer.value());
  }
  std::cout << "[DEBUG] Defining '" << node.name << "' in environment"
            << std::endl;
  interpreter->environment->define(node.name, value);
}

void ExecVisitor::visit(FunctionDeclStmt &node) {
  // Create a lambda value for the function
  auto lambda = std::make_shared<LambdaValue>(node.parameters, node.body,
                                              interpreter->environment);
  interpreter->environment->define(node.name, Value(lambda));

  // Store function definition for overload resolution and main function
  // detection
  std::vector<FunctionParameter> paramsCopy;
  for (const auto &param : node.parameters) {
    paramsCopy.push_back(param);
  }

  auto funcDef = std::make_shared<FunctionDef>(node.name, std::move(paramsCopy),
                                               std::move(node.body));
  Interpreter::functionDefinitions[node.name].push_back(funcDef);

  // Handle main function detection
  if (node.name == "main") {
    interpreter->hasMainFunction = true;
    interpreter->mainFunctionStmt = std::make_shared<FunctionDeclStmt>(node);
  }
}

void ExecVisitor::visit(ExtensionFunctionDeclStmt &node) {
  // For extension functions, we create a special function that can be called on
  // instances of the receiver type Create a lambda value for the extension
  // function, adding the receiver parameter as the first parameter
  std::vector<FunctionParameter> allParams = node.parameters;

  // Add the receiver parameter as the first parameter with a special name
  // We'll use a generic name that indicates it's the extension receiver
  FunctionParameter receiverParam(
      "this"); // Using "this" to represent the receiver
  allParams.insert(allParams.begin(), receiverParam);

  auto lambda = std::make_shared<LambdaValue>(allParams, node.body,
                                              interpreter->environment);

  // The extension function is stored with a special name that combines receiver
  // type and function name
  std::string extensionFuncName = "ext_" + node.receiverType + "_" + node.name;
  interpreter->environment->define(extensionFuncName, Value(lambda));

  // Store function definition for overload resolution (also includes receiver
  // param)
  std::vector<FunctionParameter> paramsCopy =
      allParams; // Include the receiver parameter

  auto funcDef = std::make_shared<FunctionDef>(
      extensionFuncName, std::move(paramsCopy), std::move(node.body));
  Interpreter::functionDefinitions[extensionFuncName].push_back(funcDef);
}

void ExecVisitor::visit(BlockStmt &node) {
  // Create a new environment for the block
  auto blockEnv = std::make_shared<Environment>(interpreter->environment);
  interpreter->executeBlock(node.statements, blockEnv);
}

void ExecVisitor::visit(IfStmt &node) {
  Value condition = interpreter->evaluate(*node.condition);
  bool conditionValue = false;

  if (auto *boolValue = std::get_if<bool>(&condition)) {
    conditionValue = *boolValue;
  } else {
    throw DotlinError("Runtime", "If condition must evaluate to a boolean",
                      node.line, node.column);
  }

  if (conditionValue) {
    if (node.thenBranch) {
      interpreter->execute(*node.thenBranch);
    }
  } else if (node.elseBranch && *node.elseBranch) {
    interpreter->execute(*node.elseBranch.value());
  }
}

void ExecVisitor::visit(WhileStmt &node) {
  // Execute the while loop
  while (true) {
    Value conditionValue = interpreter->evaluate(*node.condition);
    if (auto *boolVal = std::get_if<bool>(&conditionValue)) {
      if (!*boolVal) {
        break;
      }
      if (node.body) {
        interpreter->execute(*node.body);
      }
    } else {
      throw DotlinError("Runtime", "While condition must be boolean", node.line,
                        node.column);
    }
  }
}

void ExecVisitor::visit(ReturnStmt &node) {
  if (node.value) {
    interpreter->lastEvaluatedValue = interpreter->evaluate(*node.value);
  } else {
    interpreter->lastEvaluatedValue = Value();
  }

  // Throw a signal exception to stop execution and return the value
  throw std::runtime_error("RETURN_SIGNAL");
}

// Statement execution visitor implementations
void ExecVisitor::visit(ClassDeclStmt &node) {
  // Create a class definition
  auto classDef = std::make_shared<ClassDefinition>(node.name);

  // Resolve superclass if present
  if (node.superClass) {
    try {
      Value superVal = interpreter->environment->get(*node.superClass);
      if (auto *superDef =
              std::get_if<std::shared_ptr<ClassDefinition>>(&superVal)) {
        classDef->superclass = *superDef;
      } else {
        throw DotlinError(
            "Runtime", "Superclass '" + *node.superClass + "' is not a class",
            node.line, node.column);
      }
    } catch (const std::exception &e) {
      throw DotlinError("Runtime",
                        "Superclass '" + *node.superClass + "' not found",
                        node.line, node.column);
    }
  }

  // Process class members
  for (const auto &member : node.members) {
    if (auto *funcDecl = dynamic_cast<FunctionDeclStmt *>(member.get())) {
      // Create function definition
      std::vector<FunctionParameter> paramsCopy;
      for (const auto &param : funcDecl->parameters) {
        paramsCopy.push_back(param);
      }
      auto funcDef = std::make_shared<FunctionDef>(
          funcDecl->name, std::move(paramsCopy), funcDecl->body);
      classDef->methods.push_back(funcDef);
    } else if (auto *varDecl = dynamic_cast<VariableDeclStmt *>(member.get())) {
      // Store variable declaration for instantiation
      // We need to cast back to shared_ptr, but member is correct type
      // Hack: we cast the raw pointer to the derived type and make a shared
      // copy or just ref? Since 'members' is vector of shared_ptr<Statement>,
      // we can use static_pointer_cast
      auto varDeclPtr = std::static_pointer_cast<VariableDeclStmt>(member);
      classDef->fieldDecls.push_back(varDeclPtr);
      (void)varDecl; // Silence unused variable warning

      // Also add to fields metadata (optional, mainly for type checking if we
      // had it here) classDef->fields.push_back({varDecl->name,
      // varDecl->typeAnnotation.value_or(nullptr)});
    } else if (auto *ctorDecl =
                   dynamic_cast<ConstructorDeclStmt *>(member.get())) {
      // Create constructor function definition
      std::vector<FunctionParameter> paramsCopy;
      for (const auto &param : ctorDecl->parameters) {
        paramsCopy.push_back(param);
      }
      // Use "init" as internal name for constructors
      auto ctorDef = std::make_shared<FunctionDef>(
          "init", std::move(paramsCopy), ctorDecl->body);
      classDef->constructors.push_back(ctorDef);
    }
  }

  // Store the class definition in the global environment
  interpreter->environment->define(node.name, Value(classDef));
}

void ExecVisitor::visit(ForStmt &node) {
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
    throw DotlinError("Runtime", "Can only iterate over arrays", node.line,
                      node.column);
  }
}

void ExecVisitor::visit(WhenStmt &node) {
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
      return;
    }
  }

  // Check for else branch
  if (node.elseBranch) {
    interpreter->execute(**node.elseBranch);
  }
}

void ExecVisitor::visit(TryStmt &node) {
  try {
    interpreter->execute(*node.tryBlock);
  } catch (const std::runtime_error &e) {
    // Create a new scope for the catch block
    auto catchEnv = std::make_shared<Environment>(interpreter->environment);

    // Define the exception variable (remove "Runtime Error: " prefix if present
    // for cleaner access)
    std::string msg = e.what();
    catchEnv->define(node.exceptionVar, Value(msg));

    // Execute catch block in the new scope
    auto oldEnv = interpreter->environment;
    interpreter->environment = catchEnv;
    try {
      interpreter->execute(*node.catchBlock);
    } catch (...) {
      interpreter->environment = oldEnv;
      throw;
    }
    interpreter->environment = oldEnv;
  }

  // Execute finally block if present
  if (node.finallyBlock) {
    interpreter->execute(**node.finallyBlock);
  }
}

void ExecVisitor::visit(ConstructorDeclStmt &node) {
  (void)node;
  // This visitor is reached only if a constructor is defined outside a class,
  // which should be a parser error properly, or we just ignore it here.
}

// Expression visit methods (needed for complete interface but not used in
// statement execution)
void ExecVisitor::visit(LiteralExpr &node) {
  (void)node;
  // Not used in statement execution
}

void ExecVisitor::visit(StringInterpolationExpr &node) {
  (void)node;
  // Not used in statement execution
}

void ExecVisitor::visit(IdentifierExpr &node) {
  (void)node;
  // Not used in statement execution
}

void ExecVisitor::visit(LambdaExpr &node) {
  (void)node;
  // Not used in statement execution
}

void ExecVisitor::visit(BinaryExpr &node) {
  (void)node;
  // Not used in statement execution
}

void ExecVisitor::visit(UnaryExpr &node) {
  (void)node;
  // Not used in statement execution
}

void ExecVisitor::visit(CallExpr &node) {
  (void)node;
  // Not used in statement execution
}

void ExecVisitor::visit(MemberAccessExpr &node) {
  (void)node;
  // Not used in statement execution
}

void ExecVisitor::visit(ArrayLiteralExpr &node) {
  (void)node;
  // Not used in statement execution
}

void ExecVisitor::visit(ArrayAccessExpr &node) {
  (void)node;
  // Not used in statement execution
}
