#pragma once
#include "dotlin/interpreter.h"
#include "dotlin/parser.h"

namespace dotlin
{

  // Expression evaluation visitor
  struct EvalVisitor : public AstVisitor
  {
    Value result;
    Interpreter *interpreter;

    EvalVisitor(Interpreter *interp) : interpreter(interp) {}

    void visit(LiteralExpr &node) override;
    void visit(StringInterpolationExpr &node) override;
    void visit(IdentifierExpr &node) override;
    void visit(LambdaExpr &node) override;
    void visit(BinaryExpr &node) override;
    void visit(UnaryExpr &node) override;
    void visit(CallExpr &node) override;
    void visit(MemberAccessExpr &node) override;
    void visit(ArrayLiteralExpr &node) override;
    void visit(ArrayAccessExpr &node) override;

    // Statement visit methods (needed for complete interface)
    void visit(ExpressionStmt &node) override;
    void visit(VariableDeclStmt &node) override;
    void visit(FunctionDeclStmt &node) override;
    void visit(ExtensionFunctionDeclStmt &node) override;
    void visit(BlockStmt &node) override;
    void visit(IfStmt &node) override;
    void visit(WhileStmt &node) override;
    void visit(ReturnStmt &node) override;
    void visit(ClassDeclStmt &node) override;
    void visit(ForStmt &node) override;
    void visit(WhenStmt &node) override;
    void visit(TryStmt &node) override;
    void visit(ConstructorDeclStmt &node) override;
  };

  // Statement execution visitor
  struct ExecVisitor : public AstVisitor
  {
    Interpreter *interpreter;

    ExecVisitor(Interpreter *interp) : interpreter(interp) {}

    void visit(ExpressionStmt &node) override;
    void visit(VariableDeclStmt &node) override;
    void visit(FunctionDeclStmt &node) override;
    void visit(ExtensionFunctionDeclStmt &node) override;
    void visit(BlockStmt &node) override;
    void visit(IfStmt &node) override;
    void visit(WhileStmt &node) override;
    void visit(ReturnStmt &node) override;
    void visit(ClassDeclStmt &node) override;
    void visit(ForStmt &node) override;
    void visit(WhenStmt &node) override;
    void visit(TryStmt &node) override;
    void visit(ConstructorDeclStmt &node) override;

    // Expression visit methods (needed for complete interface)
    void visit(LiteralExpr &node) override;
    void visit(StringInterpolationExpr &node) override;
    void visit(IdentifierExpr &node) override;
    void visit(LambdaExpr &node) override;
    void visit(BinaryExpr &node) override;
    void visit(UnaryExpr &node) override;
    void visit(CallExpr &node) override;
    void visit(MemberAccessExpr &node) override;
    void visit(ArrayLiteralExpr &node) override;
    void visit(ArrayAccessExpr &node) override;
  };

  // Type checking visitor for expressions
  struct TypeCheckVisitor : public dotlin::AstVisitor
  {
    std::shared_ptr<dotlin::Type> result;
    TypeChecker *checker;

    TypeCheckVisitor(TypeChecker *c) : checker(c) {}

    void visit(LiteralExpr &node) override;
    void visit(StringInterpolationExpr &node) override;
    void visit(IdentifierExpr &node) override;
    void visit(LambdaExpr &node) override;
    void visit(BinaryExpr &node) override;
    void visit(UnaryExpr &node) override;
    void visit(CallExpr &node) override;
    void visit(MemberAccessExpr &node) override;
    void visit(ArrayLiteralExpr &node) override;
    void visit(ArrayAccessExpr &node) override;

    // Statement visit methods (needed for complete interface)
    void visit(ExpressionStmt &node) override;
    void visit(VariableDeclStmt &node) override;
    void visit(FunctionDeclStmt &node) override;
    void visit(ExtensionFunctionDeclStmt &node) override;
    void visit(BlockStmt &node) override;
    void visit(IfStmt &node) override;
    void visit(WhileStmt &node) override;
    void visit(ReturnStmt &node) override;
    void visit(ClassDeclStmt &node) override;
    void visit(ForStmt &node) override;
    void visit(WhenStmt &node) override;
    void visit(TryStmt &node) override;
    void visit(ConstructorDeclStmt &node) override;
  };

  // Type checking visitor for statements
  struct StmtTypeCheckVisitor : public dotlin::AstVisitor
  {
    TypeChecker *checker;

    StmtTypeCheckVisitor(TypeChecker *c) : checker(c) {}

    void visit(ExpressionStmt &node) override;
    void visit(VariableDeclStmt &node) override;
    void visit(FunctionDeclStmt &node) override;
    void visit(ExtensionFunctionDeclStmt &node) override;
    void visit(BlockStmt &node) override;
    void visit(IfStmt &node) override;
    void visit(WhileStmt &node) override;
    void visit(ReturnStmt &node) override;
    void visit(ClassDeclStmt &node) override;
    void visit(ForStmt &node) override;
    void visit(WhenStmt &node) override;
    void visit(TryStmt &node) override;
    void visit(ConstructorDeclStmt &node) override;

    // Expression visit methods (needed for complete interface)
    void visit(LiteralExpr &node) override;
    void visit(StringInterpolationExpr &node) override;
    void visit(IdentifierExpr &node) override;
    void visit(LambdaExpr &node) override;
    void visit(BinaryExpr &node) override;
    void visit(UnaryExpr &node) override;
    void visit(CallExpr &node) override;
    void visit(MemberAccessExpr &node) override;
    void visit(ArrayLiteralExpr &node) override;
    void visit(ArrayAccessExpr &node) override;
  };

  // Constant folding visitor
  struct ConstantFolderVisitor : public AstVisitor
  {
    Expression::Ptr resultExpr;
    Statement::Ptr resultStmt;

    ConstantFolderVisitor() : resultExpr(nullptr), resultStmt(nullptr) {}

    // Helper to fold an expression and return the new pointer
    Expression::Ptr fold(Expression::Ptr expr);
    // Helper to fold a statement and return the new pointer
    Statement::Ptr fold(Statement::Ptr stmt);

    void visit(LiteralExpr &node) override;
    void visit(StringInterpolationExpr &node) override;
    void visit(IdentifierExpr &node) override;
    void visit(LambdaExpr &node) override;
    void visit(BinaryExpr &node) override;
    void visit(UnaryExpr &node) override;
    void visit(CallExpr &node) override;
    void visit(MemberAccessExpr &node) override;
    void visit(ArrayLiteralExpr &node) override;
    void visit(ArrayAccessExpr &node) override;

    void visit(ExpressionStmt &node) override;
    void visit(VariableDeclStmt &node) override;
    void visit(FunctionDeclStmt &node) override;
    void visit(ExtensionFunctionDeclStmt &node) override;
    void visit(BlockStmt &node) override;
    void visit(IfStmt &node) override;
    void visit(WhileStmt &node) override;
    void visit(ReturnStmt &node) override;
    void visit(ClassDeclStmt &node) override;
    void visit(ForStmt &node) override;
    void visit(WhenStmt &node) override;
    void visit(TryStmt &node) override;
    void visit(ConstructorDeclStmt &node) override;
  };

  // Dead code elimination visitor
  struct DeadCodeEliminationVisitor : public AstVisitor
  {
    Statement::Ptr resultStmt;
    bool hasReturn;
    bool hasUnreachable;

    DeadCodeEliminationVisitor() : resultStmt(nullptr), hasReturn(false), hasUnreachable(false) {}

    // Helper to eliminate dead code from a statement and return the new pointer
    Statement::Ptr eliminate(Statement::Ptr stmt);

    void visit(BlockStmt &node) override;
    void visit(IfStmt &node) override;
    void visit(WhileStmt &node) override;
    void visit(ReturnStmt &node) override;
    void visit(ExpressionStmt &node) override;
    void visit(VariableDeclStmt &node) override;
    void visit(FunctionDeclStmt &node) override;
    void visit(ClassDeclStmt &node) override;
    void visit(ForStmt &node) override;
    void visit(WhenStmt &node) override;
    void visit(TryStmt &node) override;
    void visit(ConstructorDeclStmt &node) override;
    void visit(ExtensionFunctionDeclStmt &node) override;

    // Expression visits (needed for completeness)
    void visit(LiteralExpr &node) override;
    void visit(StringInterpolationExpr &node) override;
    void visit(IdentifierExpr &node) override;
    void visit(LambdaExpr &node) override;
    void visit(BinaryExpr &node) override;
    void visit(UnaryExpr &node) override;
    void visit(CallExpr &node) override;
    void visit(MemberAccessExpr &node) override;
    void visit(ArrayLiteralExpr &node) override;
    void visit(ArrayAccessExpr &node) override;
  };
} // namespace dotlin