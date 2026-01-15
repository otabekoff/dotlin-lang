#include "dotlin/interpreter.h"
#include "dotlin/parser.h"
#include "dotlin/visitors.h"
// #include <stdexcept>

using namespace dotlin;

// TypeChecker constructor
TypeChecker::TypeChecker(std::shared_ptr<TypeEnvironment> typeEnv,
                         std::shared_ptr<Environment> env)
    : typeEnvironment(typeEnv), environment(env) {}

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
  // Look up the identifier in the current type environment
  if (checker->typeEnvironment) {
    auto type = checker->typeEnvironment->get(node.name);
    if (type) {
      result = type;
    } else {
      // Fallback to UNKNOWN if not found
      result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
    }
  } else {
    result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
  }
}

void TypeCheckVisitor::visit(LambdaExpr &node) {
  (void)node;
  // Use the new FUNCTION type kind
  result = std::make_shared<dotlin::Type>(TypeKind::FUNCTION);
}

void TypeCheckVisitor::visit(BinaryExpr &node) {
  auto leftType = checker->checkExpression(*node.left);
  auto rightType = checker->checkExpression(*node.right);

  // Arithmetic operators
  if (node.op == TokenType::PLUS || node.op == TokenType::MINUS ||
      node.op == TokenType::MULTIPLY || node.op == TokenType::DIVIDE ||
      node.op == TokenType::MODULO) {
    if (leftType->kind == TypeKind::DOUBLE ||
        rightType->kind == TypeKind::DOUBLE) {
      result = std::make_shared<dotlin::Type>(TypeKind::DOUBLE);
    } else if (leftType->kind == TypeKind::STRING ||
               rightType->kind == TypeKind::STRING) {
      if (node.op == TokenType::PLUS) {
        result = std::make_shared<dotlin::Type>(TypeKind::STRING);
      } else {
        result = std::make_shared<dotlin::Type>(TypeKind::UNKNOWN);
      }
    } else {
      result = std::make_shared<dotlin::Type>(TypeKind::INT);
    }
    return;
  }

  // Comparison/Logical operators
  if (node.op == TokenType::EQUAL || node.op == TokenType::NOT_EQUAL ||
      node.op == TokenType::LESS || node.op == TokenType::LESS_EQUAL ||
      node.op == TokenType::GREATER || node.op == TokenType::GREATER_EQUAL ||
      node.op == TokenType::AND || node.op == TokenType::OR) {
    result = std::make_shared<dotlin::Type>(TypeKind::BOOL);
    return;
  }

  result = std::make_shared<dotlin::Type>(TypeKind::UNKNOWN);
}

void TypeCheckVisitor::visit(UnaryExpr &node) {
  auto operandType = checker->checkExpression(*node.operand);

  if (node.op == TokenType::MINUS) {
    if (operandType->kind == TypeKind::DOUBLE) {
      result = std::make_shared<dotlin::Type>(TypeKind::DOUBLE);
    } else {
      result = std::make_shared<dotlin::Type>(TypeKind::INT);
    }
    return;
  }

  if (node.op == TokenType::NOT) {
    result = std::make_shared<dotlin::Type>(TypeKind::BOOL);
    return;
  }

  result = std::make_shared<dotlin::Type>(TypeKind::UNKNOWN);
}

void TypeCheckVisitor::visit(CallExpr &node) {
  // Determine return type based on callee
  if (auto *id = dynamic_cast<IdentifierExpr *>(node.callee.get())) {
    // Look up function return type in environment
    if (checker->typeEnvironment) {
      auto type = checker->typeEnvironment->get(id->name);
      if (type) {
        result = type;
        return;
      }
    }

    // Check if it's a class constructor (class name used as function)
    // In a full implementation, we'd check if id->name is a registered class
    // For now, if it starts with uppercase, assume it's a class constructor?
    if (!id->name.empty() && std::isupper(id->name[0])) {
      // Return UNKNOWN or a Class type if implemented
      result = std::make_shared<dotlin::Type>(TypeKind::ANY);
      return;
    }
  } else if (auto *memberAccess =
                 dynamic_cast<MemberAccessExpr *>(node.callee.get())) {
    // Basic method return type inference
    if (memberAccess->property == "size" ||
        memberAccess->property == "length") {
      result = std::make_shared<dotlin::Type>(TypeKind::INT);
      return;
    }
    if (memberAccess->property == "toString") {
      result = std::make_shared<dotlin::Type>(TypeKind::STRING);
      return;
    }
  }

  result = std::make_shared<dotlin::Type>(TypeKind::UNKNOWN);
}

void TypeCheckVisitor::visit(MemberAccessExpr &node) {
  (void)node;
  result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
}

void TypeCheckVisitor::visit(ArrayLiteralExpr &node) {
  // Infer element type from elements
  std::shared_ptr<dotlin::Type> elementType =
      std::make_shared<dotlin::Type>(TypeKind::UNKNOWN);
  if (!node.elements.empty() && node.elements[0]) {
    elementType = checker->checkExpression(*node.elements[0]);
  }

  auto arrayType = std::make_shared<dotlin::Type>(TypeKind::ARRAY);
  arrayType->elementType = elementType;
  result = arrayType;
}

void TypeCheckVisitor::visit(StringInterpolationExpr &node) {
  (void)node;
  result = std::make_shared<dotlin::Type>(dotlin::TypeKind::STRING);
}

void TypeCheckVisitor::visit(ArrayAccessExpr &node) {
  auto arrayType = checker->checkExpression(*node.array);
  if (arrayType->kind == TypeKind::ARRAY && arrayType->elementType) {
    result = arrayType->elementType;
  } else {
    result = std::make_shared<dotlin::Type>(TypeKind::UNKNOWN);
  }
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
  // Variable declaration - determine the type
  std::shared_ptr<dotlin::Type> varType;

  if (node.typeAnnotation.has_value() && node.typeAnnotation.value()) {
    varType = node.typeAnnotation.value();
  } else if (node.initializer.has_value() && node.initializer.value()) {
    varType = checker->checkExpression(*node.initializer.value());
    // Store back in node for persistence/runtime use
    node.typeAnnotation = varType;
  } else {
    varType = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
  }

  // Log inferred type if it was missing
  std::cout << "Type inferred for variable '" << node.name
            << "': " << checker->typeToString(varType) << std::endl;

  // Store the variable type in the environment
  if (checker->typeEnvironment) {
    checker->typeEnvironment->define(node.name, varType);
  }
}

void StmtTypeCheckVisitor::visit(FunctionDeclStmt &node) {
  // Register function name and return type in environment
  std::shared_ptr<dotlin::Type> returnType;
  if (node.returnType.has_value() && node.returnType.value()) {
    returnType = node.returnType.value();
  } else {
    returnType = std::make_shared<dotlin::Type>(TypeKind::VOID);
  }

  if (checker->typeEnvironment) {
    checker->typeEnvironment->define(node.name, returnType);
  }

  // Check the function body in a new scope with parameters
  auto previousTypeEnv = checker->typeEnvironment;
  checker->typeEnvironment = std::make_shared<TypeEnvironment>(previousTypeEnv);

  for (const auto &param : node.parameters) {
    std::shared_ptr<dotlin::Type> paramType;
    if (param.typeAnnotation.has_value() && param.typeAnnotation.value()) {
      paramType = param.typeAnnotation.value();
    } else {
      paramType = std::make_shared<dotlin::Type>(TypeKind::ANY);
    }
    checker->typeEnvironment->define(param.name, paramType);
  }

  if (node.body) {
    checker->checkStatement(*node.body);
  }

  // Restore the previous type environment
  checker->typeEnvironment = previousTypeEnv;
}

void StmtTypeCheckVisitor::visit(BlockStmt &node) {
  // Create a new scoped type environment
  auto previousTypeEnv = checker->typeEnvironment;
  checker->typeEnvironment = std::make_shared<TypeEnvironment>(previousTypeEnv);

  // Check each statement in the block
  for (const auto &stmt : node.statements) {
    if (stmt) {
      checker->checkStatement(*stmt);
    }
  }

  // Restore the previous type environment
  checker->typeEnvironment = previousTypeEnv;
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
}

std::shared_ptr<Type> TypeChecker::checkExpression(Expression &expr) {
  TypeCheckVisitor visitor(this);
  expr.accept(visitor);
  return visitor.result;
}

void TypeChecker::checkStatement(Statement &stmt) {
  StmtTypeCheckVisitor visitor(this);
  stmt.accept(visitor);
}

std::string TypeChecker::typeToString(const std::shared_ptr<Type> &type) {
  return ::dotlin::typeToString(type);
}