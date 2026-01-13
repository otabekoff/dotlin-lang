// Parser for Dotlin - Kotlin-like language implementation in C++
#pragma once
#include "dotlin/lexer.h"
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace dotlin
{

  // Forward declarations for AST nodes
  struct Expression;
  struct Statement;
  struct LiteralExpr;
  struct IdentifierExpr;
  struct BinaryExpr;
  struct UnaryExpr;
  struct CallExpr;
  struct MemberAccessExpr;
  struct ArrayAccessExpr;
  struct ArrayLiteralExpr;
  struct ExpressionStmt;
  struct VariableDeclStmt;
  struct FunctionDeclStmt;
  struct BlockStmt;
  struct ReturnStmt;
  struct IfStmt;

  // Base class for all AST nodes
  struct AstNode
  {
    virtual ~AstNode() = default;
    virtual void accept(class AstVisitor &visitor) = 0;
    size_t line;
    size_t column;

    AstNode(size_t l, size_t c) : line(l), column(c) {}
  };

  // Expression base class
  struct Expression : AstNode
  {
    using Ptr = std::unique_ptr<Expression>;
    Expression(size_t l, size_t c) : AstNode(l, c) {}
    virtual ~Expression() = default;
  };

  // Statement base class
  struct Statement : AstNode
  {
    using Ptr = std::unique_ptr<Statement>;
    Statement(size_t l, size_t c) : AstNode(l, c) {}
    virtual ~Statement() = default;
  };

  // AST Node Types
  struct Program
  {
    std::vector<std::unique_ptr<Statement>> statements;
  };

  struct Identifier
  {
    std::string name;
    size_t line;
    size_t column;
  };

  struct Literal
  {
    std::variant<int, double, bool, std::string> value;
    size_t line;
    size_t column;
  };

  struct BinaryExpression
  {
    std::unique_ptr<Expression> left;
    TokenType op;
    std::unique_ptr<Expression> right;
    size_t line;
    size_t column;
  };

  struct UnaryExpression
  {
    TokenType op;
    std::unique_ptr<Expression> operand;
    size_t line;
    size_t column;
  };

  struct CallExpression
  {
    std::unique_ptr<Expression> callee;
    std::vector<std::unique_ptr<Expression>> arguments;
    size_t line;
    size_t column;
  };

  struct MemberAccess
  {
    std::unique_ptr<Expression> object;
    Identifier property;
    size_t line;
    size_t column;
  };

  struct VariableDeclaration
  {
    bool isVal; // true for val, false for var
    Identifier name;
    std::optional<std::unique_ptr<Expression>> initializer;
    size_t line;
    size_t column;
  };

  struct FunctionDeclaration
  {
    Identifier name;
    std::vector<Identifier> parameters;
    std::unique_ptr<Expression> body;
    size_t line;
    size_t column;
  };

  struct IfExpression
  {
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Expression> thenBranch;
    std::optional<std::unique_ptr<Expression>> elseBranch;
    size_t line;
    size_t column;
  };

  struct BlockStatement
  {
    std::vector<std::unique_ptr<Statement>> statements;
    size_t line;
    size_t column;
  };

  struct ReturnStatement
  {
    std::unique_ptr<Expression> value;
    size_t line;
    size_t column;
  };

  // Visitor pattern for AST traversal
  class AstVisitor
  {
  public:
    virtual ~AstVisitor() = default;
    virtual void visit(LiteralExpr &node) = 0;
    virtual void visit(IdentifierExpr &node) = 0;
    virtual void visit(BinaryExpr &node) = 0;
    virtual void visit(UnaryExpr &node) = 0;
    virtual void visit(CallExpr &node) = 0;
    virtual void visit(MemberAccessExpr &node) = 0;
    virtual void visit(ArrayAccessExpr &node) = 0;
    virtual void visit(ExpressionStmt &node) = 0;
    virtual void visit(VariableDeclStmt &node) = 0;
    virtual void visit(FunctionDeclStmt &node) = 0;
    virtual void visit(BlockStmt &node) = 0;
    virtual void visit(ReturnStmt &node) = 0;
    virtual void visit(ArrayLiteralExpr &node) = 0;
    virtual void visit(IfStmt &node) = 0;
  };

  // Concrete expression types
  struct LiteralExpr : Expression
  {
    std::variant<int, double, bool, std::string> value;
    LiteralExpr(std::variant<int, double, bool, std::string> v, size_t l,
                size_t c)
        : Expression(l, c), value(std::move(v)) {}

    void accept(AstVisitor &visitor) override { visitor.visit(*this); }
  };

  struct IdentifierExpr : Expression
  {
    std::string name;
    IdentifierExpr(std::string n, size_t l, size_t c)
        : Expression(l, c), name(std::move(n)) {}

    void accept(AstVisitor &visitor) override { visitor.visit(*this); }
  };

  struct BinaryExpr : Expression
  {
    Expression::Ptr left;
    TokenType op;
    Expression::Ptr right;
    BinaryExpr(Expression::Ptr l, TokenType o, Expression::Ptr r, size_t l_,
               size_t c)
        : Expression(l_, c), left(std::move(l)), op(o), right(std::move(r)) {}

    void accept(AstVisitor &visitor) override { visitor.visit(*this); }
  };

  struct UnaryExpr : Expression
  {
    TokenType op;
    Expression::Ptr operand;
    UnaryExpr(TokenType o, Expression::Ptr operandParam, size_t l, size_t c)
        : Expression(l, c), op(o), operand(std::move(operandParam)) {}

    void accept(AstVisitor &visitor) override { visitor.visit(*this); }
  };

  struct CallExpr : Expression
  {
    Expression::Ptr callee;
    std::vector<Expression::Ptr> arguments;
    CallExpr(Expression::Ptr calleeParam, std::vector<Expression::Ptr> args, size_t l,
             size_t c)
        : Expression(l, c), callee(std::move(calleeParam)),
          arguments(std::move(args)) {}

    void accept(AstVisitor &visitor) override { visitor.visit(*this); }
  };

  struct MemberAccessExpr : Expression
  {
    Expression::Ptr object;
    std::string property;
    MemberAccessExpr(Expression::Ptr obj, std::string prop, size_t l, size_t c)
        : Expression(l, c), object(std::move(obj)), property(std::move(prop)) {}

    void accept(AstVisitor &visitor) override { visitor.visit(*this); }
  };

  struct ArrayAccessExpr : Expression
  {
    Expression::Ptr array;
    Expression::Ptr index;
    ArrayAccessExpr(Expression::Ptr arr, Expression::Ptr idx, size_t l, size_t c)
        : Expression(l, c), array(std::move(arr)), index(std::move(idx)) {}

    void accept(AstVisitor &visitor) override { visitor.visit(*this); }
  };

  struct ArrayLiteralExpr : Expression
  {
    std::vector<Expression::Ptr> elements;
    ArrayLiteralExpr(std::vector<Expression::Ptr> elems, size_t l, size_t c)
        : Expression(l, c), elements(std::move(elems)) {}

    void accept(AstVisitor &visitor) override { visitor.visit(*this); }
  };

  // Concrete statement types
  struct ExpressionStmt : Statement
  {
    Expression::Ptr expression;
    ExpressionStmt(Expression::Ptr expr, size_t l, size_t c)
        : Statement(l, c), expression(std::move(expr)) {}

    void accept(AstVisitor &visitor) override { visitor.visit(*this); }
  };

  struct VariableDeclStmt : Statement
  {
    bool isVal; // true for val, false for var
    std::string name;
    std::optional<Expression::Ptr> initializer;
    VariableDeclStmt(bool is_val, std::string n,
                     std::optional<Expression::Ptr> init, size_t l, size_t c)
        : Statement(l, c), isVal(is_val), name(std::move(n)),
          initializer(std::move(init)) {}

    void accept(AstVisitor &visitor) override { visitor.visit(*this); }
  };

  struct FunctionDeclStmt : Statement
  {
    std::string name;
    std::vector<std::string> parameters;
    Statement::Ptr body;
    FunctionDeclStmt(std::string n, std::vector<std::string> params,
                     Statement::Ptr b, size_t l, size_t c)
        : Statement(l, c), name(std::move(n)), parameters(std::move(params)),
          body(std::move(b)) {}

    void accept(AstVisitor &visitor) override { visitor.visit(*this); }
  };

  struct BlockStmt : Statement
  {
    std::vector<Statement::Ptr> statements;
    BlockStmt(std::vector<Statement::Ptr> stmts, size_t l, size_t c)
        : Statement(l, c), statements(std::move(stmts)) {}

    void accept(AstVisitor &visitor) override { visitor.visit(*this); }
  };

  struct ReturnStmt : Statement
  {
    Expression::Ptr value;
    ReturnStmt(Expression::Ptr val, size_t l, size_t c)
        : Statement(l, c), value(std::move(val)) {}

    void accept(AstVisitor &visitor) override { visitor.visit(*this); }
  };

  struct IfStmt : Statement
  {
    Expression::Ptr condition;
    Statement::Ptr thenBranch;
    std::optional<Statement::Ptr> elseBranch;
    IfStmt(Expression::Ptr cond, Statement::Ptr then_branch,
           std::optional<Statement::Ptr> else_branch, size_t l, size_t c)
        : Statement(l, c), condition(std::move(cond)),
          thenBranch(std::move(then_branch)), elseBranch(std::move(else_branch))
    {
    }

    void accept(AstVisitor &visitor) override { visitor.visit(*this); }
  };

  Program parse(const std::vector<Token> &tokens);

  // Parser helper functions
  std::unique_ptr<Statement> parseStatement(const std::vector<Token> &tokens,
                                            size_t &pos);
  std::unique_ptr<Statement>
  parseVariableDeclaration(const std::vector<Token> &tokens, size_t &pos);
  std::unique_ptr<Statement>
  parseFunctionDeclaration(const std::vector<Token> &tokens, size_t &pos);
  std::unique_ptr<Statement> parseIfStatement(const std::vector<Token> &tokens,
                                              size_t &pos);
  std::unique_ptr<Statement>
  parseReturnStatement(const std::vector<Token> &tokens, size_t &pos);
  std::unique_ptr<Statement> parseBlockStatement(const std::vector<Token> &tokens,
                                                 size_t &pos);
  std::unique_ptr<Statement>
  parseExpressionStatement(const std::vector<Token> &tokens, size_t &pos);
  std::unique_ptr<Expression> parseExpression(const std::vector<Token> &tokens,
                                              size_t &pos);
  std::unique_ptr<Expression>
  parseComparisonExpression(const std::vector<Token> &tokens, size_t &pos);
  std::unique_ptr<Expression>
  parseAdditiveExpression(const std::vector<Token> &tokens, size_t &pos);
  std::unique_ptr<Expression>
  parseMultiplicativeExpression(const std::vector<Token> &tokens, size_t &pos);
  std::unique_ptr<Expression>
  parsePostfixExpression(const std::vector<Token> &tokens, size_t &pos);
  std::unique_ptr<Expression>
  parsePrimaryExpression(const std::vector<Token> &tokens, size_t &pos);

} // namespace dotlin