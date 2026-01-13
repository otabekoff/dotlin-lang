// Interpreter for Dotlin - Kotlin-like language implementation in C++
#pragma once
#include "dotlin/parser.h"
#include <any>
#include <string>
#include <unordered_map>
#include <vector>

namespace dotlin
{

  // Runtime value representation
  using Value = std::variant<int, double, bool, std::string>;

  // Environment for variable bindings
  struct Environment
  {
    std::unordered_map<std::string, Value> values;
    Environment *enclosing = nullptr;

    Environment(Environment *parent = nullptr) : enclosing(parent) {}

    void define(const std::string &name, Value value);
    Value get(const std::string &name);
    void assign(const std::string &name, Value value);
  };

  // Interpreter class
  class Interpreter
  {
  public:
    Interpreter();
    Value interpret(const Program &program);
    Value interpret(const Program &program, const std::vector<std::string> &args);

    // Visitor pattern implementation
    void visit(LiteralExpr &node);
    void visit(IdentifierExpr &node);
    void visit(BinaryExpr &node);
    void visit(UnaryExpr &node);
    void visit(CallExpr &node);
    void visit(MemberAccessExpr &node);
    void visit(ExpressionStmt &node);
    void visit(VariableDeclStmt &node);
    void visit(FunctionDeclStmt &node);
    void visit(BlockStmt &node);
    void visit(ReturnStmt &node);
    void visit(IfStmt &node);

  private:
    Environment globals;
    Environment *environment;
    bool hasMainFunction;
    Statement::Ptr mainFunctionStmt;          // Store reference to main function if found
    std::vector<std::string> commandLineArgs; // Store command-line arguments

    Value evaluate(Expression &expr);
    void execute(Statement &stmt);
    Value executeBlock(const std::vector<Statement::Ptr> &statements,
                       Environment *env);
    std::string valueToString(const Value &value);
  };

  Value interpret(const Program &program);
  Value interpret(const Program &program, const std::vector<std::string> &args);

} // namespace dotlin