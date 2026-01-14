#include "dotlin/interpreter.h"
#include "dotlin/parser.h"
#include "dotlin/visitors.h"
// #include <iostream>
#include <stdexcept>

using namespace dotlin;

// Type checking visitor for expressions - Implementation
void TypeCheckVisitor::visit(LiteralExpr &node) {
  (void)node;
  // Determine type from literal value
  if (std::holds_alternative<int>(node.value)) {
    result = std::make_shared<dotlin::Type>(dotlin::TypeKind::INT);
  } else if (std::holds_alternative<double>(node.value)) {
    result = std::make_shared<dotlin::Type>(dotlin::TypeKind::DOUBLE);
  } else if (std::holds_alternative<bool>(node.value)) {
    result = std::make_shared<dotlin::Type>(dotlin::TypeKind::BOOL);
  } else if (std::holds_alternative<std::string>(node.value)) {
    result = std::make_shared<dotlin::Type>(dotlin::TypeKind::STRING);
  } else {
    result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
  }
}

void TypeCheckVisitor::visit(IdentifierExpr &node) {
  (void)node;
  // Look up the identifier in the current environment
  if (checker->environment) {
    try {
      // For now, we'll just return UNKNOWN type
      // In a full implementation, we'd look up the variable's type
      result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
    } catch (...) {
      result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
    }
  } else {
    result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
  }
}

void TypeCheckVisitor::visit(LambdaExpr &node) {
  (void)node;
  // For now, just return UNKNOWN type for lambdas
  // In a full implementation, we'd analyze the lambda body and parameters
  result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
}

void TypeCheckVisitor::visit(BinaryExpr &node) {
  auto leftType = checker->checkExpression(*node.left);
  auto rightType = checker->checkExpression(*node.right);

  if (node.op == TokenType::PLUS) {
    // String concatenation or numeric addition
    if (leftType->kind == dotlin::TypeKind::STRING ||
        rightType->kind == dotlin::TypeKind::STRING) {
      result = std::make_shared<dotlin::Type>(dotlin::TypeKind::STRING);
    } else if (leftType->kind == dotlin::TypeKind::INT &&
               rightType->kind == dotlin::TypeKind::INT) {
      result = std::make_shared<dotlin::Type>(dotlin::TypeKind::INT);
    } else {
      result = std::make_shared<dotlin::Type>(dotlin::TypeKind::DOUBLE);
    }
  } else if (node.op == TokenType::EQUAL || node.op == TokenType::NOT_EQUAL) {
    result = std::make_shared<dotlin::Type>(dotlin::TypeKind::BOOL);
  } else if (node.op == TokenType::LESS || node.op == TokenType::LESS_EQUAL ||
             node.op == TokenType::GREATER ||
             node.op == TokenType::GREATER_EQUAL) {
    result = std::make_shared<dotlin::Type>(dotlin::TypeKind::BOOL);
  } else {
    // For other operations, return the more general numeric type
    if (leftType->kind == dotlin::TypeKind::DOUBLE ||
        rightType->kind == dotlin::TypeKind::DOUBLE) {
      result = std::make_shared<dotlin::Type>(dotlin::TypeKind::DOUBLE);
    } else {
      result = std::make_shared<dotlin::Type>(dotlin::TypeKind::INT);
    }
  }
}

void TypeCheckVisitor::visit(UnaryExpr &node) {
  auto operandType = checker->checkExpression(*node.operand);

  if (node.op == TokenType::MINUS) {
    result = operandType;
  } else if (node.op == TokenType::NOT) {
    result = std::make_shared<dotlin::Type>(dotlin::TypeKind::BOOL);
  } else {
    result = operandType;
  }
}

void TypeCheckVisitor::visit(CallExpr &node) {
  // For now, just return UNKNOWN type for function calls
  // In a full implementation, we'd look up the function signature
  result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
}

void TypeCheckVisitor::visit(MemberAccessExpr &node) {
  auto objectType = checker->checkExpression(*node.object);

  if (objectType->kind == dotlin::TypeKind::UNKNOWN) {
    // For now, we'll just return UNKNOWN
    // In a full implementation, we'd look up the member's type
    result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
  } else {
    result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
  }
}

void TypeCheckVisitor::visit(ArrayLiteralExpr &node) {
  (void)node;
  // Create an array type
  auto arrayType = std::make_shared<dotlin::Type>(dotlin::TypeKind::ARRAY);

  // For now, we'll just set the element type to UNKNOWN
  // In a full implementation, we'd analyze all elements to find common type
  arrayType->elementType =
      std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);

  result = arrayType;
}

void TypeCheckVisitor::visit(ArrayAccessExpr &node) {
  (void)node;
  auto arrayType = checker->checkExpression(*node.array);
  auto indexType = checker->checkExpression(*node.index);

  if (arrayType->kind == dotlin::TypeKind::ARRAY &&
      indexType->kind == dotlin::TypeKind::INT) {
    result = arrayType;
  } else {
    result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
  }
}

// Statement visit methods (needed for complete interface but not used in
// expression type checking)
void TypeCheckVisitor::visit(ExpressionStmt &node) {
  (void)node;
  // Not used in expression type checking
  result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
}

void TypeCheckVisitor::visit(VariableDeclStmt &node) {
  (void)node;
  // Not used in expression type checking
  result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
}

void TypeCheckVisitor::visit(FunctionDeclStmt &node) {
  (void)node;
  // Not used in expression type checking
  result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
}

void TypeCheckVisitor::visit(BlockStmt &node) {
  (void)node;
  // Not used in expression type checking
  result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
}

void TypeCheckVisitor::visit(IfStmt &node) {
  (void)node;
  // Not used in expression type checking
  result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
}

void TypeCheckVisitor::visit(WhileStmt &node) {
  (void)node;
  // Not used in expression type checking
  result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
}

void TypeCheckVisitor::visit(ReturnStmt &node) {
  (void)node;
  // Not used in expression type checking
  result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
}

void TypeCheckVisitor::visit(ClassDeclStmt &node) {
  (void)node;
  // Not used in expression type checking
  result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
}

void TypeCheckVisitor::visit(ForStmt &node) {
  (void)node;
  // Not used in expression type checking
  result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
}

void TypeCheckVisitor::visit(WhenStmt &node) {
  (void)node;
  // Not used in expression type checking
  result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
}

void TypeCheckVisitor::visit(TryStmt &node) {
  (void)node;
  // Not used in expression type checking
  result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
}

void TypeCheckVisitor::visit(ConstructorDeclStmt &node) {
  (void)node;
  // Not used in expression type checking
  result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
}

// Statement type checking visitor - Implementation
void StmtTypeCheckVisitor::visit(ExpressionStmt &node) {
  checker->checkExpression(*node.expression);
}

void StmtTypeCheckVisitor::visit(VariableDeclStmt &node) {
  std::shared_ptr<dotlin::Type> varType;
  if (node.initializer) {
    varType = checker->checkExpression(*node.initializer.value());
  } else {
    varType = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
  }

  // Store the variable type in the environment
  if (checker->environment) {
    // For now, we'll just skip this
    // In a full implementation, we'd store the type
  }
}

void StmtTypeCheckVisitor::visit(FunctionDeclStmt &node) {
  (void)node;
  // For now, we'll just skip this
  // In a full implementation, we'd analyze the function body
}

void StmtTypeCheckVisitor::visit(BlockStmt &node) {
  (void)node;
  // Create a new environment for the block
  auto previousEnv = checker->environment;
  // For now, we'll just use the same environment
  // In a full implementation, we'd create a new environment

  for (const auto &stmt : node.statements) {
    checker->checkStatement(*stmt);
  }

  checker->environment = previousEnv;
}

void StmtTypeCheckVisitor::visit(IfStmt &node) {
  auto conditionType = checker->checkExpression(*node.condition);

  if (conditionType->kind != dotlin::TypeKind::BOOL) {
    // Type error: condition must be boolean
  }

  checker->checkStatement(*node.thenBranch);

  if (node.elseBranch) {
    checker->checkStatement(*node.elseBranch.value());
  }
}

void StmtTypeCheckVisitor::visit(WhileStmt &node) {
  auto conditionType = checker->checkExpression(*node.condition);

  if (conditionType->kind != dotlin::TypeKind::BOOL) {
    // Type error: condition must be boolean
  }

  checker->checkStatement(*node.body);
}

void StmtTypeCheckVisitor::visit(ReturnStmt &node) {
  if (node.value) {
    auto returnType = checker->checkExpression(*node.value);
    // Check if return type matches function return type
    // For now, we'll just skip this check
  }
}

void StmtTypeCheckVisitor::visit(ClassDeclStmt &node) {
  (void)node;
  // For now, we'll just skip this
  // In a full implementation, we'd create a class type

  // Check the constructor and methods
  // TODO: Handle constructor and methods from members
}

void StmtTypeCheckVisitor::visit(ForStmt &node) {
  (void)node;
  // TODO: Implement for loop type checking
  throw std::runtime_error("For loop type checking not yet implemented");
}

void StmtTypeCheckVisitor::visit(WhenStmt &node) {
  (void)node;
  // TODO: Implement when statement type checking
  throw std::runtime_error("When statement type checking not yet implemented");
}

void StmtTypeCheckVisitor::visit(TryStmt &node) {
  (void)node;
  // TODO: Implement try-catch type checking
  throw std::runtime_error("Try-catch type checking not yet implemented");
}

void StmtTypeCheckVisitor::visit(ConstructorDeclStmt &node) {
  (void)node;
  // TODO: Implement constructor declaration type checking
  throw std::runtime_error(
      "Constructor declaration type checking not yet implemented");
}

void StmtTypeCheckVisitor::visit(LiteralExpr &node) {
  (void)node;
  // Not used in statement type checking
}

void StmtTypeCheckVisitor::visit(IdentifierExpr &node) {
  (void)node;
  // Not used in statement type checking
}

void StmtTypeCheckVisitor::visit(LambdaExpr &node) {
  (void)node;
  // Not used in statement type checking
}

void StmtTypeCheckVisitor::visit(BinaryExpr &node) {
  (void)node;
  // Not used in statement type checking
}

void StmtTypeCheckVisitor::visit(UnaryExpr &node) {
  (void)node;
  // Not used in statement type checking
}

void StmtTypeCheckVisitor::visit(CallExpr &node) {
  (void)node;
  // Not used in statement type checking
}

void StmtTypeCheckVisitor::visit(MemberAccessExpr &node) {
  (void)node;
  // Not used in statement type checking
}

void StmtTypeCheckVisitor::visit(ArrayLiteralExpr &node) {
  (void)node;
  // Not used in statement type checking
}

void StmtTypeCheckVisitor::visit(ArrayAccessExpr &node) {
  (void)node;
  // Not used in statement type checking
}
