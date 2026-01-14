#include "dotlin/parser.h"
#include "dotlin/visitors.h"
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
    value = interpreter->evaluate(*node.initializer.value());
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

void ExecVisitor::visit(ClassDeclStmt &node) {
  // Create a class definition
  auto classDef = std::make_shared<ClassDefinition>(node.name);
  // TODO: Handle constructor and methods from members
  // For now, just create an empty class

  // Store the class definition in the global environment
  interpreter->environment->define(node.name, Value(classDef));
}

void ExecVisitor::visit(ForStmt &node) {
  (void)node;
  // TODO: Implement for loop
  throw std::runtime_error("For loops not yet implemented");
}

void ExecVisitor::visit(WhenStmt &node) {
  (void)node;
  // TODO: Implement when statement
  throw std::runtime_error("When statements not yet implemented");
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
