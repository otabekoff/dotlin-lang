#include "dotlin/interpreter.h"
#include "dotlin/parser.h"
#include "dotlin/visitors.h"
// #include <stdexcept>

using namespace dotlin;

// TypeChecker constructor
TypeChecker::TypeChecker(std::shared_ptr<Environment> env) : environment(env) {}

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
  // Look up the identifier in the current environment
  (void)node;
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

  // For now, just return UNKNOWN type
  // In a full implementation, we'd determine the result type based on operation
  (void)leftType;
  (void)rightType;
  result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
}

void TypeCheckVisitor::visit(UnaryExpr &node) {
  (void)node;
  result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
}

void TypeCheckVisitor::visit(CallExpr &node) {
  (void)node;
  result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
}

void TypeCheckVisitor::visit(MemberAccessExpr &node) {
  (void)node;
  result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
}

void TypeCheckVisitor::visit(ArrayLiteralExpr &node) {
  (void)node;
  result = std::make_shared<dotlin::Type>(dotlin::TypeKind::ARRAY);
}

void TypeCheckVisitor::visit(StringInterpolationExpr &node) {
  (void)node;
  result = std::make_shared<dotlin::Type>(dotlin::TypeKind::STRING);
}

void TypeCheckVisitor::visit(ArrayAccessExpr &node) {
  (void)node;
  result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
}

// Add the missing statement visitor implementations to TypeCheckVisitor
void TypeCheckVisitor::visit(ExpressionStmt &node) {
  (void)node;
  result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
}

void TypeCheckVisitor::visit(VariableDeclStmt &node) {
  (void)node;
  result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
}

void TypeCheckVisitor::visit(FunctionDeclStmt &node) {
  (void)node;
  result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
}

void TypeCheckVisitor::visit(BlockStmt &node) {
  (void)node;
  result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
}

void TypeCheckVisitor::visit(ReturnStmt &node) {
  (void)node;
  result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
}

void TypeCheckVisitor::visit(IfStmt &node) {
  (void)node;
  result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
}

void TypeCheckVisitor::visit(WhileStmt &node) {
  (void)node;
  result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
}

void TypeCheckVisitor::visit(ForStmt &node) {
  (void)node;
  result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
}

void TypeCheckVisitor::visit(WhenStmt &node) {
  (void)node;
  result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
}

void TypeCheckVisitor::visit(TryStmt &node) {
  (void)node;
  result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
}

void TypeCheckVisitor::visit(ConstructorDeclStmt &node) {
  (void)node;
  result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
}

void TypeCheckVisitor::visit(ClassDeclStmt &node) {
  (void)node;
  result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
}

// Statement type checking visitor - Implementation
void StmtTypeCheckVisitor::visit(ExpressionStmt &node) {
  checker->checkExpression(*node.expression);
}

void StmtTypeCheckVisitor::visit(VariableDeclStmt &node) {
  // Variable declaration - just check the initializer type
  std::shared_ptr<dotlin::Type> varType;
  if (node.initializer) {
    varType = checker->checkExpression(*node.initializer.value());
  } else {
    varType = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
  }

  // Store the variable type in the environment
  if (checker->environment) {
    // For now, we'll just skip this
    (void)varType;
  }
}

void StmtTypeCheckVisitor::visit(FunctionDeclStmt &node) {
  (void)node;
  // For now, just skip function declarations
}

void StmtTypeCheckVisitor::visit(BlockStmt &node) {
  // Check each statement in the block
  for (const auto &stmt : node.statements) {
    if (stmt) {
      checker->checkStatement(*stmt);
    }
  }
}

void StmtTypeCheckVisitor::visit(IfStmt &node) {
  // Check condition
  if (node.condition) {
    checker->checkExpression(*node.condition);
  }

  // Check then branch
  if (node.thenBranch) {
    checker->checkStatement(*node.thenBranch);
  }

  // Check else branch if present
  if (node.elseBranch.has_value() && node.elseBranch.value()) {
    checker->checkStatement(*node.elseBranch.value());
  }
}

void StmtTypeCheckVisitor::visit(WhileStmt &node) {
  // Check condition
  if (node.condition) {
    checker->checkExpression(*node.condition);
  }

  // Check body
  if (node.body) {
    checker->checkStatement(*node.body);
  }
}

void StmtTypeCheckVisitor::visit(ReturnStmt &node) {
  // Check return value if present
  if (node.value) {
    checker->checkExpression(*node.value);
  }
}

void StmtTypeCheckVisitor::visit(ClassDeclStmt &node) {
  (void)node;
  // For now, just skip class declarations
}

void StmtTypeCheckVisitor::visit(ForStmt &node) {
  // Check iterable
  if (node.iterable) {
    checker->checkExpression(*node.iterable);
  }

  // Check body
  if (node.body) {
    checker->checkStatement(*node.body);
  }
}

void StmtTypeCheckVisitor::visit(WhenStmt &node) {
  // Check subject
  if (node.subject) {
    checker->checkExpression(*node.subject);
  }

  // Check each branch
  for (const auto &branch : node.branches) {
    if (branch.first) {
      checker->checkExpression(*branch.first);
    }
    if (branch.second) {
      checker->checkStatement(*branch.second);
    }
  }

  // Check else branch if present
  if (node.elseBranch.has_value() && node.elseBranch.value()) {
    checker->checkStatement(*node.elseBranch.value());
  }
}

void StmtTypeCheckVisitor::visit(TryStmt &node) {
  // Check try block
  if (node.tryBlock) {
    checker->checkStatement(*node.tryBlock);
  }

  // Check catch block if present
  if (node.catchBlock) {
    checker->checkStatement(*node.catchBlock);
  }

  // Check finally block if present
  if (node.finallyBlock.has_value() && node.finallyBlock.value()) {
    checker->checkStatement(*node.finallyBlock.value());
  }
}

void StmtTypeCheckVisitor::visit(ConstructorDeclStmt &node) {
  (void)node;
  // For now, just skip constructor declarations
}

// Add the missing StmtTypeCheckVisitor implementations for expressions
void StmtTypeCheckVisitor::visit(LiteralExpr &node) {
  (void)node; // Not used in statement type checking
}

void StmtTypeCheckVisitor::visit(IdentifierExpr &node) {
  (void)node; // Not used in statement type checking
}

void StmtTypeCheckVisitor::visit(LambdaExpr &node) {
  (void)node; // Not used in statement type checking
}

void StmtTypeCheckVisitor::visit(BinaryExpr &node) {
  (void)node; // Not used in statement type checking
}

void StmtTypeCheckVisitor::visit(UnaryExpr &node) {
  (void)node; // Not used in statement type checking
}

void StmtTypeCheckVisitor::visit(CallExpr &node) {
  (void)node; // Not used in statement type checking
}

void StmtTypeCheckVisitor::visit(MemberAccessExpr &node) {
  (void)node; // Not used in statement type checking
}

void StmtTypeCheckVisitor::visit(ArrayLiteralExpr &node) {
  (void)node; // Not used in statement type checking
}

void StmtTypeCheckVisitor::visit(StringInterpolationExpr &node) {
  (void)node; // Not used in statement type checking
}

void StmtTypeCheckVisitor::visit(ArrayAccessExpr &node) {
  (void)node; // Not used in statement type checking
} // namespace dotlin
