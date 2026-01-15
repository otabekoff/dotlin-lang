// Parser for Dotlin - Kotlin-like language implementation in C++
#pragma once
#include "dotlin/lexer.h"
#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace dotlin
{

  // Define the primitive types supported by literals
  using LiteralValue = std::variant<int, int64_t, double, bool, std::string>;

  // Type system - moved to top to be available for expressions
  enum class TypeKind
  {
    INT,
    LONG,
    DOUBLE,
    BOOL,
    STRING,
    ARRAY,
    VOID,
    UNKNOWN,
    ANY,
    FUNCTION
  };

  struct Type
  {
    TypeKind kind;
    std::shared_ptr<Type> elementType; // For arrays
    std::vector<std::shared_ptr<Type>>
        genericTypes; // For generic type parameters

    Type(TypeKind k) : kind(k), elementType(nullptr), genericTypes() {}
    Type(TypeKind k, std::shared_ptr<Type> elemType)
        : kind(k), elementType(elemType), genericTypes() {}
    Type(TypeKind k, std::shared_ptr<Type> elemType,
         std::vector<std::shared_ptr<Type>> genTypes)
        : kind(k), elementType(elemType), genericTypes(std::move(genTypes)) {}

    bool isCompatibleWith(const Type &other) const
    {
      if (kind == other.kind)
      {
        // For generic types, check if generic parameters are compatible
        if (kind == TypeKind::ARRAY && !genericTypes.empty() &&
            !other.genericTypes.empty())
        {
          if (genericTypes.size() != other.genericTypes.size())
            return false;
          for (size_t i = 0; i < genericTypes.size(); ++i)
          {
            if (!genericTypes[i]->isCompatibleWith(*other.genericTypes[i]))
              return false;
          }
          return true;
        }
        return true;
      }
      if (kind == TypeKind::UNKNOWN || other.kind == TypeKind::UNKNOWN)
        return true;
      if (kind == TypeKind::INT && other.kind == TypeKind::DOUBLE)
        return true; // int can be promoted to double
      if (kind == TypeKind::DOUBLE && other.kind == TypeKind::INT)
        return true; // double can be demoted to int
      if (kind == TypeKind::ARRAY && other.kind == TypeKind::ARRAY)
      {
        if (elementType && other.elementType)
        {
          return elementType->isCompatibleWith(*other.elementType);
        }
        return true; // Arrays with unknown element types are compatible
      }
      return false;
    }
  };

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
  struct LambdaExpr;
  struct ExpressionStmt;
  struct VariableDeclStmt;
  struct FunctionDeclStmt;
  struct BlockStmt;
  struct ReturnStmt;
  struct IfStmt;
  struct WhileStmt;
  struct ForStmt;
  struct WhenStmt;
  struct TryStmt;
  struct ClassDeclStmt;
  struct ConstructorDeclStmt;

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
    std::shared_ptr<Type> exprType;
    Expression(size_t l, size_t c)
        : AstNode(l, c), exprType(std::make_shared<Type>(TypeKind::UNKNOWN)) {}
    virtual ~Expression() = default;
  };

  // Statement base class
  struct Statement : AstNode
  {
    using Ptr = std::shared_ptr<Statement>;
    Statement(size_t l, size_t c) : AstNode(l, c) {}
    virtual ~Statement() = default;
  };

  // AST Node Types
  struct Program
  {
    std::vector<Statement::Ptr> statements;
  };

  struct Identifier
  {
    std::string name;
    size_t line;
    size_t column;
  };

  struct Literal
  {
    LiteralValue value;
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

  struct TypeAnnotation
  {
    std::string typeName;
    std::optional<std::string> genericType; // For generic types like Array<Int>
    size_t line;
    size_t column;

    TypeAnnotation(std::string type, size_t l, size_t c)
        : typeName(std::move(type)), line(l), column(c) {}
    TypeAnnotation(std::string type, std::optional<std::string> genType, size_t l,
                   size_t c)
        : typeName(std::move(type)), genericType(std::move(genType)), line(l),
          column(c) {}
  };

  struct VariableDeclaration
  {
    bool isVal; // true for val, false for var
    Identifier name;
    std::optional<TypeAnnotation> typeAnnotation;
    std::optional<std::unique_ptr<Expression>> initializer;
    size_t line;
    size_t column;
  };

  struct ParameterDeclaration
  {
    Identifier name;
    std::optional<TypeAnnotation> typeAnnotation;
    size_t line;
    size_t column;

    ParameterDeclaration(Identifier n, size_t l, size_t c)
        : name(std::move(n)), line(l), column(c) {}
    ParameterDeclaration(Identifier n, std::optional<TypeAnnotation> type,
                         size_t l, size_t c)
        : name(std::move(n)), typeAnnotation(std::move(type)), line(l),
          column(c) {}
  };

  struct FunctionDeclaration
  {
    Identifier name;
    std::vector<ParameterDeclaration> parameters;
    std::unique_ptr<Expression> body;
    std::optional<TypeAnnotation> returnType;
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

  struct WhileStatement
  {
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Statement> body;
    size_t line;
    size_t column;
  };

  struct ForStatement
  {
    std::string variable;
    std::unique_ptr<Expression> iterable;
    std::unique_ptr<Statement> body;
    size_t line;
    size_t column;
  };

  struct WhenStatement
  {
    std::unique_ptr<Expression> subject;
    std::vector<
        std::pair<std::unique_ptr<Expression>, std::unique_ptr<Statement>>>
        branches;
    std::optional<std::unique_ptr<Statement>> elseBranch;
    size_t line;
    size_t column;
  };

  struct TryStatement
  {
    std::unique_ptr<Statement> tryBlock;
    std::string exceptionVar;
    std::unique_ptr<Statement> catchBlock;
    std::optional<std::unique_ptr<Statement>> finallyBlock;
    size_t line;
    size_t column;
  };

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
  std::unique_ptr<Statement> parseWhileStatement(const std::vector<Token> &tokens,
                                                 size_t &pos);
  std::unique_ptr<Statement> parseForStatement(const std::vector<Token> &tokens,
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
  std::unique_ptr<Expression>
  parseStringInterpolation(const std::string &strValue, size_t line,
                           size_t column);

  // Forward declarations for visitor pattern
  struct StringInterpolationExpr;
  struct LiteralExpr;
  struct IdentifierExpr;
  struct BinaryExpr;
  struct UnaryExpr;
  struct CallExpr;
  struct MemberAccessExpr;
  struct ArrayAccessExpr;
  struct ExpressionStmt;
  struct VariableDeclStmt;
  struct FunctionDeclStmt;
  struct BlockStmt;
  struct ReturnStmt;
  struct ArrayLiteralExpr;
  struct LambdaExpr;
  struct IfStmt;
  struct WhileStmt;
  struct ForStmt;
  struct WhenStmt;
  struct TryStmt;
  struct ConstructorDeclStmt;
  struct ClassDeclStmt;
  struct ExtensionFunctionDeclStmt;

  // Visitor pattern for AST traversal
  class AstVisitor
  {
  public:
    virtual ~AstVisitor() = default;
    virtual void visit(LiteralExpr &node) = 0;
    virtual void visit(StringInterpolationExpr &node) = 0;
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
    virtual void visit(LambdaExpr &node) = 0;
    virtual void visit(IfStmt &node) = 0;
    virtual void visit(WhileStmt &node) = 0;
    virtual void visit(ForStmt &node) = 0;
    virtual void visit(WhenStmt &node) = 0;
    virtual void visit(TryStmt &node) = 0;
    virtual void visit(ConstructorDeclStmt &node) = 0;
    virtual void visit(ClassDeclStmt &node) = 0;
    virtual void visit(ExtensionFunctionDeclStmt &node) = 0;
  };

  // Concrete expression types
  struct LiteralExpr : Expression
  {
    LiteralValue value;
    LiteralExpr(LiteralValue v, size_t l, size_t c)
        : Expression(l, c), value(std::move(v)) {}

    void accept(AstVisitor &visitor) override { visitor.visit(*this); }
  };

  struct StringInterpolationExpr : Expression
  {
    std::vector<Expression::Ptr>
        parts; // Alternating string literals and expressions
    StringInterpolationExpr(std::vector<Expression::Ptr> p, size_t l, size_t c)
        : Expression(l, c), parts(std::move(p)) {}

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
    CallExpr(Expression::Ptr calleeParam, std::vector<Expression::Ptr> args,
             size_t l, size_t c)
        : Expression(l, c), callee(std::move(calleeParam)),
          arguments(std::move(args))
    {
      if (!callee)
      {
        std::cout << "CRITICAL: CallExpr callee is NULL!" << std::endl;
      }
    }

    void accept(AstVisitor &visitor) override { visitor.visit(*this); }
  };

  struct MemberAccessExpr : Expression
  {
    Expression::Ptr object;
    std::string property;
    MemberAccessExpr(Expression::Ptr obj, std::string prop, size_t l, size_t c)
        : Expression(l, c), object(std::move(obj)), property(std::move(prop))
    {
      if (!object)
      {
        std::cout << "CRITICAL: MemberAccessExpr object is null for property "
                  << property << std::endl;
      }
    }

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

  struct FunctionParameter
  {
    std::string name;
    std::optional<std::shared_ptr<Type>> typeAnnotation;
    FunctionParameter(std::string n)
        : name(std::move(n)), typeAnnotation(std::nullopt) {}
    FunctionParameter(std::string n, std::optional<std::shared_ptr<Type>> type)
        : name(std::move(n)), typeAnnotation(std::move(type)) {}
  };

  struct ArrayLiteralExpr : Expression
  {
    std::vector<Expression::Ptr> elements;
    ArrayLiteralExpr(std::vector<Expression::Ptr> elems, size_t l, size_t c)
        : Expression(l, c), elements(std::move(elems)) {}

    void accept(AstVisitor &visitor) override { visitor.visit(*this); }
  };

  struct LambdaExpr : Expression
  {
    std::vector<FunctionParameter> parameters;
    Statement::Ptr body;
    std::optional<std::shared_ptr<Type>> returnType;

    LambdaExpr(std::vector<FunctionParameter> params, Statement::Ptr b, size_t l,
               size_t c)
        : Expression(l, c), parameters(std::move(params)), body(std::move(b)),
          returnType(std::nullopt) {}

    LambdaExpr(std::vector<FunctionParameter> params, Statement::Ptr b,
               std::optional<std::shared_ptr<Type>> retType, size_t l, size_t c)
        : Expression(l, c), parameters(std::move(params)), body(std::move(b)),
          returnType(std::move(retType)) {}

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
    std::optional<std::shared_ptr<Type>> typeAnnotation;
    std::optional<Expression::Ptr> initializer;
    VariableDeclStmt(bool is_val, std::string n,
                     std::optional<Expression::Ptr> init, size_t l, size_t c)
        : Statement(l, c), isVal(is_val), name(std::move(n)),
          typeAnnotation(std::nullopt), initializer(std::move(init)) {}

    VariableDeclStmt(bool is_val, std::string n,
                     std::optional<std::shared_ptr<Type>> type,
                     std::optional<Expression::Ptr> init, size_t l, size_t c)
        : Statement(l, c), isVal(is_val), name(std::move(n)),
          typeAnnotation(std::move(type)), initializer(std::move(init)) {}

    void accept(AstVisitor &visitor) override { visitor.visit(*this); }
  };

  struct FunctionDeclStmt : Statement
  {
    std::string name;
    std::vector<FunctionParameter> parameters;
    Statement::Ptr body;
    std::optional<std::shared_ptr<Type>> returnType;
    FunctionDeclStmt(std::string n, std::vector<std::string> params,
                     Statement::Ptr b, size_t l, size_t c)
        : Statement(l, c), name(std::move(n)), body(std::move(b)),
          returnType(std::nullopt)
    {
      // Convert string parameters to FunctionParameter with no type annotation
      for (const auto &param : params)
      {
        parameters.emplace_back(param);
      }
    }

    FunctionDeclStmt(std::string n, std::vector<FunctionParameter> params,
                     Statement::Ptr b,
                     std::optional<std::shared_ptr<Type>> retType, size_t l,
                     size_t c)
        : Statement(l, c), name(std::move(n)), parameters(std::move(params)),
          body(std::move(b)), returnType(std::move(retType)) {}

    void accept(AstVisitor &visitor) override { visitor.visit(*this); }
  };

  // AST node for extension function declarations
  struct ExtensionFunctionDeclStmt : Statement
  {
    std::string receiverType; // The type being extended
    std::string name;         // The name of the extension function
    std::vector<FunctionParameter> parameters;
    Statement::Ptr body;
    std::optional<std::shared_ptr<Type>> returnType;

    ExtensionFunctionDeclStmt(std::string recvrType, std::string funcName,
                              std::vector<FunctionParameter> params,
                              Statement::Ptr funcBody,
                              std::optional<std::shared_ptr<Type>> retType,
                              size_t l, size_t c)
        : Statement(l, c), receiverType(std::move(recvrType)), name(std::move(funcName)),
          parameters(std::move(params)), body(std::move(funcBody)),
          returnType(std::move(retType)) {}

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

  struct WhileStmt : Statement
  {
    Expression::Ptr condition;
    Statement::Ptr body;
    WhileStmt(Expression::Ptr cond, Statement::Ptr body_stmt, size_t l, size_t c)
        : Statement(l, c), condition(std::move(cond)),
          body(std::move(body_stmt)) {}

    void accept(AstVisitor &visitor) override { visitor.visit(*this); }
  };

  struct ForStmt : Statement
  {
    std::string variable;
    Expression::Ptr iterable;
    Statement::Ptr body;
    ForStmt(std::string var, Expression::Ptr iter, Statement::Ptr body_stmt,
            size_t l, size_t c)
        : Statement(l, c), variable(std::move(var)), iterable(std::move(iter)),
          body(std::move(body_stmt)) {}

    void accept(AstVisitor &visitor) override { visitor.visit(*this); }
  };

  struct WhenStmt : Statement
  {
    Expression::Ptr subject;
    std::vector<std::pair<Expression::Ptr, Statement::Ptr>> branches;
    std::optional<Statement::Ptr> elseBranch;
    WhenStmt(Expression::Ptr subj,
             std::vector<std::pair<Expression::Ptr, Statement::Ptr>> brs,
             std::optional<Statement::Ptr> else_br, size_t l, size_t c)
        : Statement(l, c), subject(std::move(subj)), branches(std::move(brs)),
          elseBranch(std::move(else_br)) {}

    void accept(AstVisitor &visitor) override { visitor.visit(*this); }
  };

  struct ConstructorDeclStmt : Statement
  {
    std::vector<FunctionParameter> parameters;
    Statement::Ptr body;
    std::optional<std::string>
        className; // Name of the class this constructor belongs to

    ConstructorDeclStmt(std::vector<FunctionParameter> params,
                        Statement::Ptr ctorBody, size_t l, size_t c)
        : Statement(l, c), parameters(std::move(params)),
          body(std::move(ctorBody)), className(std::nullopt) {}

    ConstructorDeclStmt(std::vector<FunctionParameter> params,
                        Statement::Ptr ctorBody,
                        std::optional<std::string> clsName, size_t l, size_t c)
        : Statement(l, c), parameters(std::move(params)),
          body(std::move(ctorBody)), className(std::move(clsName)) {}

    void accept(AstVisitor &visitor) override { visitor.visit(*this); }
  };

  struct ClassDeclStmt : Statement
  {
    std::string name;
    std::vector<Statement::Ptr> members; // Properties and methods
    std::optional<std::string> superClass;

    ClassDeclStmt(std::string className, std::vector<Statement::Ptr> classMembers,
                  size_t l, size_t c)
        : Statement(l, c), name(std::move(className)),
          members(std::move(classMembers)), superClass(std::nullopt) {}

    ClassDeclStmt(std::string className, std::vector<Statement::Ptr> classMembers,
                  std::optional<std::string> superCls, size_t l, size_t c)
        : Statement(l, c), name(std::move(className)),
          members(std::move(classMembers)), superClass(std::move(superCls)) {}

    void accept(AstVisitor &visitor) override { visitor.visit(*this); }
  };

  struct TryStmt : Statement
  {
    Statement::Ptr tryBlock;
    std::string exceptionVar;
    Statement::Ptr catchBlock;
    std::optional<Statement::Ptr> finallyBlock;
    TryStmt(Statement::Ptr try_block, std::string ex_var,
            Statement::Ptr catch_block,
            std::optional<Statement::Ptr> finally_block, size_t l, size_t c)
        : Statement(l, c), tryBlock(std::move(try_block)),
          exceptionVar(std::move(ex_var)), catchBlock(std::move(catch_block)),
          finallyBlock(std::move(finally_block)) {}

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
  std::unique_ptr<Statement> parseWhileStatement(const std::vector<Token> &tokens,
                                                 size_t &pos);
  std::unique_ptr<Statement> parseForStatement(const std::vector<Token> &tokens,
                                               size_t &pos);
  std::unique_ptr<Statement> parseWhenStatement(const std::vector<Token> &tokens,
                                                size_t &pos);
  std::unique_ptr<Statement> parseTryStatement(const std::vector<Token> &tokens,
                                               size_t &pos);
  std::unique_ptr<Statement>
  parseExpressionStatement(const std::vector<Token> &tokens, size_t &pos);
  std::unique_ptr<Statement>
  parseClassDeclaration(const std::vector<Token> &tokens, size_t &pos);
  std::unique_ptr<Expression> parseExpression(const std::vector<Token> &tokens,
                                              size_t &pos);
  std::unique_ptr<Expression>
  parseAssignmentExpression(const std::vector<Token> &tokens, size_t &pos);
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