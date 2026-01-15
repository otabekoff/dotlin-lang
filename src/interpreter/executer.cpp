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
  // Execute the expression for its side effects
  interpreter->evaluate(*node.expression);
}

void ExecVisitor::visit(VariableDeclStmt &node) {
  Value value;
  if (node.initializer) {
    if (!node.initializer.value()) {
      std::cout << "CRITICAL: Initializer is NULL for " << node.name
                << std::endl;
    }
    value = interpreter->evaluate(*node.initializer.value());
    std::cout << "DEBUG: Evaluated initializer for " << node.name << std::endl;
  }
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
    throw std::runtime_error("If condition must evaluate to a boolean");
  }

  if (conditionValue) {
    interpreter->execute(*node.thenBranch);
  } else if (node.elseBranch) {
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
      interpreter->execute(*node.body);
    } else {
      throw std::runtime_error("While condition must be boolean");
    }
  }
}

void ExecVisitor::visit(ReturnStmt &node) {
  Value returnValue;
  if (node.value) {
    returnValue = interpreter->evaluate(*node.value);
  }

  // Throw an exception to propagate the return value
  std::string returnMsg = "RETURN:" + dotlin::valueToString(returnValue);
  throw std::runtime_error(returnMsg);
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
        throw std::runtime_error("Superclass '" + *node.superClass +
                                 "' is not a class");
      }
    } catch (const std::exception &e) {
      throw std::runtime_error("Superclass '" + *node.superClass +
                               "' not found");
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
      // TODO: Handle constructors
      (void)ctorDecl; // Silence unused variable warning
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
    throw std::runtime_error("Can only iterate over arrays");
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
  (void)node;
  // TODO: Implement try-catch
  throw std::runtime_error("Try-catch not yet implemented");
}

void ExecVisitor::visit(ConstructorDeclStmt &node) {
  (void)node;
  // TODO: Implement constructor declaration
  throw std::runtime_error("Constructor declarations not yet implemented");
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
