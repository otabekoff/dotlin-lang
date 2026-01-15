#include "dotlin/visitors.h"
// #include <iostream>
#include <variant>

using namespace dotlin;

namespace dotlin
{

  static LiteralValue valueToLiteralVariant(const Value &v)
  {
    if (std::holds_alternative<int>(v))
      return std::get<int>(v);
    if (std::holds_alternative<int64_t>(v))
      return std::get<int64_t>(v);
    if (std::holds_alternative<double>(v))
      return std::get<double>(v);
    if (std::holds_alternative<bool>(v))
      return std::get<bool>(v);
    if (std::holds_alternative<std::string>(v))
      return std::get<std::string>(v);
    return 0;
  }

  Expression::Ptr ConstantFolderVisitor::fold(Expression::Ptr expr)
  {
    if (!expr)
      return nullptr;
    resultExpr = nullptr;
    expr->accept(*this);
    if (resultExpr)
    {
      // std::cout << "FOLDED Expression at L" << expr->line << ":" <<
      // expr->column << std::endl;
      Expression::Ptr res = std::move(resultExpr);
      resultExpr = nullptr;
      return res;
    }
    return expr;
  }

  Statement::Ptr ConstantFolderVisitor::fold(Statement::Ptr stmt)
  {
    if (!stmt)
      return nullptr;
    resultStmt = nullptr;
    stmt->accept(*this);
    if (resultStmt)
    {
      // std::cout << "FOLDED Statement at L" << stmt->line << ":" << stmt->column
      // << std::endl;
      Statement::Ptr res = resultStmt; // Use assignment for shared_ptr
      resultStmt = nullptr;
      return res;
    }
    return stmt;
  }

  void ConstantFolderVisitor::visit(LiteralExpr &node)
  {
    (void)node;
    resultExpr = nullptr;
  }
  void ConstantFolderVisitor::visit(IdentifierExpr &node)
  {
    (void)node;
    resultExpr = nullptr;
  }

  void ConstantFolderVisitor::visit(StringInterpolationExpr &node)
  {
    bool allLiterals = true;
    std::string resultStr = "";
    for (auto &part : node.parts)
    {
      if (!part)
        continue;
      part = fold(std::move(part));
      if (!part)
      {
        allLiterals = false;
        continue;
      }
      auto lit = dynamic_cast<LiteralExpr *>(part.get());
      if (lit)
      {
        if (std::holds_alternative<std::string>(lit->value))
          resultStr += std::get<std::string>(lit->value);
        else if (std::holds_alternative<int>(lit->value))
          resultStr += std::to_string(std::get<int>(lit->value));
        else if (std::holds_alternative<int64_t>(lit->value))
          resultStr += std::to_string(std::get<int64_t>(lit->value));
        else if (std::holds_alternative<double>(lit->value))
          resultStr += std::to_string(std::get<double>(lit->value));
        else if (std::holds_alternative<bool>(lit->value))
          resultStr += std::get<bool>(lit->value) ? "true" : "false";
        else
          allLiterals = false;
      }
      else
        allLiterals = false;
    }
    if (allLiterals)
      resultExpr = std::make_unique<LiteralExpr>(
          valueToLiteralVariant(Value(resultStr)), node.line, node.column);
    else
      resultExpr = nullptr;
  }

  void ConstantFolderVisitor::visit(LambdaExpr &node)
  {
    if (node.body)
      node.body = fold(node.body); // shared_ptr
    resultExpr = nullptr;
  }

  void ConstantFolderVisitor::visit(BinaryExpr &node)
  {
    node.left = fold(std::move(node.left));
    node.right = fold(std::move(node.right));
    if (!node.left || !node.right)
    {
      resultExpr = nullptr;
      return;
    }
    auto leftLit = dynamic_cast<LiteralExpr *>(node.left.get());
    auto rightLit = dynamic_cast<LiteralExpr *>(node.right.get());
    if (leftLit && rightLit)
    {
      if (std::holds_alternative<int>(leftLit->value) &&
          std::holds_alternative<int>(rightLit->value))
      {
        int l = std::get<int>(leftLit->value);
        int r = std::get<int>(rightLit->value);
        switch (node.op)
        {
        case TokenType::PLUS:
          resultExpr = std::make_unique<LiteralExpr>(
              valueToLiteralVariant(Value(l + r)), node.line, node.column);
          return;
        case TokenType::MINUS:
          resultExpr = std::make_unique<LiteralExpr>(
              valueToLiteralVariant(Value(l - r)), node.line, node.column);
          return;
        case TokenType::MULTIPLY:
          resultExpr = std::make_unique<LiteralExpr>(
              valueToLiteralVariant(Value(l * r)), node.line, node.column);
          return;
        case TokenType::DIVIDE:
          if (r != 0)
          {
            resultExpr = std::make_unique<LiteralExpr>(
                valueToLiteralVariant(Value(l / r)), node.line, node.column);
            return;
          }
          break;
        case TokenType::MODULO:
          if (r != 0)
          {
            resultExpr = std::make_unique<LiteralExpr>(
                valueToLiteralVariant(Value(l % r)), node.line, node.column);
            return;
          }
          break;
        case TokenType::EQUAL:
          resultExpr = std::make_unique<LiteralExpr>(
              valueToLiteralVariant(Value(l == r)), node.line, node.column);
          return;
        case TokenType::NOT_EQUAL:
          resultExpr = std::make_unique<LiteralExpr>(
              valueToLiteralVariant(Value(l != r)), node.line, node.column);
          return;
        case TokenType::GREATER:
          resultExpr = std::make_unique<LiteralExpr>(
              valueToLiteralVariant(Value(l > r)), node.line, node.column);
          return;
        case TokenType::GREATER_EQUAL:
          resultExpr = std::make_unique<LiteralExpr>(
              valueToLiteralVariant(Value(l >= r)), node.line, node.column);
          return;
        case TokenType::LESS:
          resultExpr = std::make_unique<LiteralExpr>(
              valueToLiteralVariant(Value(l < r)), node.line, node.column);
          return;
        case TokenType::LESS_EQUAL:
          resultExpr = std::make_unique<LiteralExpr>(
              valueToLiteralVariant(Value(l <= r)), node.line, node.column);
          return;
        default:
          break;
        }
      }
      else if ((std::holds_alternative<int64_t>(leftLit->value) ||
                std::holds_alternative<int>(leftLit->value)) &&
               (std::holds_alternative<int64_t>(rightLit->value) ||
                std::holds_alternative<int>(rightLit->value)))
      {
        int64_t l = std::holds_alternative<int64_t>(leftLit->value)
                        ? std::get<int64_t>(leftLit->value)
                        : static_cast<int64_t>(std::get<int>(leftLit->value));
        int64_t r = std::holds_alternative<int64_t>(rightLit->value)
                        ? std::get<int64_t>(rightLit->value)
                        : static_cast<int64_t>(std::get<int>(rightLit->value));
        switch (node.op)
        {
        case TokenType::PLUS:
          resultExpr = std::make_unique<LiteralExpr>(
              valueToLiteralVariant(Value(l + r)), node.line, node.column);
          return;
        case TokenType::MINUS:
          resultExpr = std::make_unique<LiteralExpr>(
              valueToLiteralVariant(Value(l - r)), node.line, node.column);
          return;
        case TokenType::MULTIPLY:
          resultExpr = std::make_unique<LiteralExpr>(
              valueToLiteralVariant(Value(l * r)), node.line, node.column);
          return;
        case TokenType::DIVIDE:
          if (r != 0)
          {
            resultExpr = std::make_unique<LiteralExpr>(
                valueToLiteralVariant(Value(l / r)), node.line, node.column);
            return;
          }
          break;
        case TokenType::EQUAL:
          resultExpr = std::make_unique<LiteralExpr>(
              valueToLiteralVariant(Value(l == r)), node.line, node.column);
          return;
        case TokenType::NOT_EQUAL:
          resultExpr = std::make_unique<LiteralExpr>(
              valueToLiteralVariant(Value(l != r)), node.line, node.column);
          return;
        case TokenType::GREATER:
          resultExpr = std::make_unique<LiteralExpr>(
              valueToLiteralVariant(Value(l > r)), node.line, node.column);
          return;
        case TokenType::GREATER_EQUAL:
          resultExpr = std::make_unique<LiteralExpr>(
              valueToLiteralVariant(Value(l >= r)), node.line, node.column);
          return;
        case TokenType::LESS:
          resultExpr = std::make_unique<LiteralExpr>(
              valueToLiteralVariant(Value(l < r)), node.line, node.column);
          return;
        case TokenType::LESS_EQUAL:
          resultExpr = std::make_unique<LiteralExpr>(
              valueToLiteralVariant(Value(l <= r)), node.line, node.column);
          return;
        default:
          break;
        }
      }
      else if ((std::holds_alternative<double>(leftLit->value) ||
                std::holds_alternative<int>(leftLit->value) ||
                std::holds_alternative<int64_t>(leftLit->value)) &&
               (std::holds_alternative<double>(rightLit->value) ||
                std::holds_alternative<int>(rightLit->value) ||
                std::holds_alternative<int64_t>(rightLit->value)))
      {
        double l =
            std::holds_alternative<double>(leftLit->value)
                ? std::get<double>(leftLit->value)
                : (std::holds_alternative<int64_t>(leftLit->value)
                       ? static_cast<double>(std::get<int64_t>(leftLit->value))
                       : static_cast<double>(std::get<int>(leftLit->value)));
        double r =
            std::holds_alternative<double>(rightLit->value)
                ? std::get<double>(rightLit->value)
                : (std::holds_alternative<int64_t>(rightLit->value)
                       ? static_cast<double>(std::get<int64_t>(rightLit->value))
                       : static_cast<double>(std::get<int>(rightLit->value)));
        switch (node.op)
        {
        case TokenType::PLUS:
          resultExpr = std::make_unique<LiteralExpr>(
              valueToLiteralVariant(Value(l + r)), node.line, node.column);
          return;
        case TokenType::MINUS:
          resultExpr = std::make_unique<LiteralExpr>(
              valueToLiteralVariant(Value(l - r)), node.line, node.column);
          return;
        case TokenType::MULTIPLY:
          resultExpr = std::make_unique<LiteralExpr>(
              valueToLiteralVariant(Value(l * r)), node.line, node.column);
          return;
        case TokenType::DIVIDE:
          if (r != 0.0)
          {
            resultExpr = std::make_unique<LiteralExpr>(
                valueToLiteralVariant(Value(l / r)), node.line, node.column);
            return;
          }
          break;
        case TokenType::EQUAL:
          resultExpr = std::make_unique<LiteralExpr>(
              valueToLiteralVariant(Value(l == r)), node.line, node.column);
          return;
        case TokenType::NOT_EQUAL:
          resultExpr = std::make_unique<LiteralExpr>(
              valueToLiteralVariant(Value(l != r)), node.line, node.column);
          return;
        case TokenType::GREATER:
          resultExpr = std::make_unique<LiteralExpr>(
              valueToLiteralVariant(Value(l > r)), node.line, node.column);
          return;
        case TokenType::GREATER_EQUAL:
          resultExpr = std::make_unique<LiteralExpr>(
              valueToLiteralVariant(Value(l >= r)), node.line, node.column);
          return;
        case TokenType::LESS:
          resultExpr = std::make_unique<LiteralExpr>(
              valueToLiteralVariant(Value(l < r)), node.line, node.column);
          return;
        case TokenType::LESS_EQUAL:
          resultExpr = std::make_unique<LiteralExpr>(
              valueToLiteralVariant(Value(l <= r)), node.line, node.column);
          return;
        default:
          break;
        }
      }
      else if (std::holds_alternative<std::string>(leftLit->value) &&
               std::holds_alternative<std::string>(rightLit->value) &&
               node.op == TokenType::PLUS)
      {
        resultExpr = std::make_unique<LiteralExpr>(
            valueToLiteralVariant(Value(std::get<std::string>(leftLit->value) +
                                        std::get<std::string>(rightLit->value))),
            node.line, node.column);
        return;
      }
      else if (std::holds_alternative<bool>(leftLit->value) &&
               std::holds_alternative<bool>(rightLit->value))
      {
        bool l = std::get<bool>(leftLit->value);
        bool r = std::get<bool>(rightLit->value);
        switch (node.op)
        {
        case TokenType::AND:
          resultExpr = std::make_unique<LiteralExpr>(
              valueToLiteralVariant(Value(l && r)), node.line, node.column);
          return;
        case TokenType::OR:
          resultExpr = std::make_unique<LiteralExpr>(
              valueToLiteralVariant(Value(l || r)), node.line, node.column);
          return;
        case TokenType::EQUAL:
          resultExpr = std::make_unique<LiteralExpr>(
              valueToLiteralVariant(Value(l == r)), node.line, node.column);
          return;
        case TokenType::NOT_EQUAL:
          resultExpr = std::make_unique<LiteralExpr>(
              valueToLiteralVariant(Value(l != r)), node.line, node.column);
          return;
        default:
          break;
        }
      }
    }
    resultExpr = nullptr;
  }

  void ConstantFolderVisitor::visit(UnaryExpr &node)
  {
    node.operand = fold(std::move(node.operand));
    if (!node.operand)
    {
      resultExpr = nullptr;
      return;
    }
    auto lit = dynamic_cast<LiteralExpr *>(node.operand.get());
    if (lit)
    {
      if (node.op == TokenType::MINUS)
      {
        if (std::holds_alternative<int>(lit->value))
        {
          resultExpr = std::make_unique<LiteralExpr>(
              valueToLiteralVariant(Value(-std::get<int>(lit->value))), node.line,
              node.column);
          return;
        }
        else if (std::holds_alternative<int64_t>(lit->value))
        {
          resultExpr = std::make_unique<LiteralExpr>(
              valueToLiteralVariant(Value(-std::get<int64_t>(lit->value))),
              node.line, node.column);
          return;
        }
        else if (std::holds_alternative<double>(lit->value))
        {
          resultExpr = std::make_unique<LiteralExpr>(
              valueToLiteralVariant(Value(-std::get<double>(lit->value))),
              node.line, node.column);
          return;
        }
      }
      else if (node.op == TokenType::NOT)
      {
        if (std::holds_alternative<bool>(lit->value))
        {
          resultExpr = std::make_unique<LiteralExpr>(
              valueToLiteralVariant(Value(!std::get<bool>(lit->value))),
              node.line, node.column);
          return;
        }
      }
    }
    resultExpr = nullptr;
  }

  void ConstantFolderVisitor::visit(CallExpr &node)
  {
    node.callee = fold(std::move(node.callee));
    for (auto &arg : node.arguments)
      arg = fold(std::move(arg));
    resultExpr = nullptr;
  }

  void ConstantFolderVisitor::visit(MemberAccessExpr &node)
  {
    node.object = fold(std::move(node.object));
    resultExpr = nullptr;
  }
  void ConstantFolderVisitor::visit(ArrayLiteralExpr &node)
  {
    for (auto &element : node.elements)
      element = fold(std::move(element));
    resultExpr = nullptr;
  }
  void ConstantFolderVisitor::visit(ArrayAccessExpr &node)
  {
    node.array = fold(std::move(node.array));
    node.index = fold(std::move(node.index));
    resultExpr = nullptr;
  }
  void ConstantFolderVisitor::visit(ExpressionStmt &node)
  {
    node.expression = fold(std::move(node.expression));
    resultStmt = nullptr;
  }
  void ConstantFolderVisitor::visit(VariableDeclStmt &node)
  {
    if (node.initializer.has_value())
      node.initializer = fold(std::move(node.initializer.value()));
    resultStmt = nullptr;
  }

  void ConstantFolderVisitor::visit(FunctionDeclStmt &node)
  {
    if (node.body)
      node.body = fold(node.body);
    resultStmt = nullptr;
  }

  void ConstantFolderVisitor::visit(BlockStmt &node)
  {
    for (auto &stmt : node.statements)
      if (stmt)
        stmt = fold(stmt);
    resultStmt = nullptr;
  }

  void ConstantFolderVisitor::visit(IfStmt &node)
  {
    node.condition = fold(std::move(node.condition));
    if (node.thenBranch)
      node.thenBranch = fold(node.thenBranch);
    if (node.elseBranch.has_value() && node.elseBranch.value())
      node.elseBranch = fold(node.elseBranch.value());
    if (!node.condition)
    {
      resultStmt = nullptr;
      return;
    }
    auto cLit = dynamic_cast<LiteralExpr *>(node.condition.get());
    if (cLit && std::holds_alternative<bool>(cLit->value))
    {
      if (std::get<bool>(cLit->value))
        resultStmt = node.thenBranch;
      else
      {
        if (node.elseBranch.has_value())
          resultStmt = node.elseBranch.value();
        else
          resultStmt = std::make_shared<BlockStmt>(std::vector<Statement::Ptr>(),
                                                   node.line, node.column);
      }
      return;
    }
    resultStmt = nullptr;
  }

  void ConstantFolderVisitor::visit(WhileStmt &node)
  {
    node.condition = fold(std::move(node.condition));
    if (node.body)
      node.body = fold(node.body);
    if (!node.condition)
    {
      resultStmt = nullptr;
      return;
    }
    auto cLit = dynamic_cast<LiteralExpr *>(node.condition.get());
    if (cLit && std::holds_alternative<bool>(cLit->value) &&
        !std::get<bool>(cLit->value))
    {
      resultStmt = std::make_shared<BlockStmt>(std::vector<Statement::Ptr>(),
                                               node.line, node.column);
      return;
    }
    resultStmt = nullptr;
  }

  void ConstantFolderVisitor::visit(ReturnStmt &node)
  {
    if (node.value)
      node.value = fold(std::move(node.value));
    resultStmt = nullptr;
  }
  void ConstantFolderVisitor::visit(ClassDeclStmt &node)
  {
    (void)node;
    resultStmt = nullptr;
  }
  void ConstantFolderVisitor::visit(ForStmt &node)
  {
    node.iterable = fold(std::move(node.iterable));
    if (node.body)
      node.body = fold(node.body);
    resultStmt = nullptr;
  }
  void ConstantFolderVisitor::visit(WhenStmt &node)
  {
    (void)node;
    resultStmt = nullptr;
  }
  void ConstantFolderVisitor::visit(TryStmt &node)
  {
    if (node.tryBlock)
      node.tryBlock = fold(node.tryBlock);
    resultStmt = nullptr;
  }
  void ConstantFolderVisitor::visit(ConstructorDeclStmt &node)
  {
    if (node.body)
      node.body = fold(node.body);
    resultStmt = nullptr;
  }

  void ConstantFolderVisitor::visit(ExtensionFunctionDeclStmt &node)
  {
    if (node.body)
      node.body = fold(node.body);
    resultStmt = nullptr;
  }

} // namespace dotlin
