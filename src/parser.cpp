// src/parser.cpp
#include "dotlin/parser.h"
#include <cstring>
#include <iostream>
#include <stdexcept>

namespace dotlin
{

  // Parser implementation for Dotlin
  Program parse(const std::vector<Token> &tokens)
  {
    Program program;
    size_t pos = 0;

    while (pos < tokens.size() && tokens[pos].type != TokenType::EOF_TOKEN)
    {
      auto stmt = parseStatement(tokens, pos);
      if (stmt)
      {
        program.statements.push_back(std::move(stmt));
      }
      else
      {
        // If we couldn't parse a statement, advance the position to avoid infinite loop
        pos++;
      }
    }

    return program;
  }

  std::unique_ptr<Statement> parseStatement(const std::vector<Token> &tokens,
                                            size_t &pos)
  {
    if (pos >= tokens.size())
      return nullptr;

    switch (tokens[pos].type)
    {
    case TokenType::VAL:
    case TokenType::VAR:
      return parseVariableDeclaration(tokens, pos);
    case TokenType::FUN:
      return parseFunctionDeclaration(tokens, pos);
    case TokenType::IF:
      return parseIfStatement(tokens, pos);
    case TokenType::RETURN:
      return parseReturnStatement(tokens, pos);
    case TokenType::LBRACE:
      return parseBlockStatement(tokens, pos);
    default:
      return parseExpressionStatement(tokens, pos);
    }
  }

  std::unique_ptr<Statement>
  parseVariableDeclaration(const std::vector<Token> &tokens, size_t &pos)
  {
    bool isVal = tokens[pos].type == TokenType::VAL;
    pos++; // consume val/var token

    if (pos >= tokens.size() || tokens[pos].type != TokenType::IDENTIFIER)
    {
      // Error handling would go here
      return nullptr;
    }

    std::string name = tokens[pos].text;
    pos++; // consume identifier

    std::optional<Expression::Ptr> initializer = std::nullopt;
    if (pos < tokens.size() && tokens[pos].type == TokenType::ASSIGN)
    {
      pos++; // consume assignment token
      auto expr = parseExpression(tokens, pos);
      if (expr)
      {
        initializer = std::move(expr);
      }
    }

    // Use the last token we consumed for position info, or default to 1,1
    size_t line = (pos > 0) ? tokens[pos - 1].line : 1;
    size_t col = (pos > 0) ? tokens[pos - 1].column : 1;
    return std::make_unique<VariableDeclStmt>(isVal, name, std::move(initializer),
                                              line, col);
  }

  std::unique_ptr<Expression> parseExpression(const std::vector<Token> &tokens,
                                              size_t &pos)
  {
    return parseComparisonExpression(tokens, pos);
  }

  std::unique_ptr<Expression>
  parseComparisonExpression(const std::vector<Token> &tokens, size_t &pos)
  {
    auto left = parseAdditiveExpression(tokens, pos);

    while (pos < tokens.size() &&
           (tokens[pos].type == TokenType::EQUAL ||
            tokens[pos].type == TokenType::NOT_EQUAL ||
            tokens[pos].type == TokenType::LESS ||
            tokens[pos].type == TokenType::LESS_EQUAL ||
            tokens[pos].type == TokenType::GREATER ||
            tokens[pos].type == TokenType::GREATER_EQUAL))
    {
      TokenType op = tokens[pos].type;
      pos++; // consume operator
      auto right = parseAdditiveExpression(tokens, pos);
      if (right)
      {
        left = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right),
                                            tokens[pos - 2].line,
                                            tokens[pos - 2].column);
      }
    }

    return left;
  }

  std::unique_ptr<Expression>
  parseAdditiveExpression(const std::vector<Token> &tokens, size_t &pos)
  {
    auto left = parseMultiplicativeExpression(tokens, pos);

    while (pos < tokens.size() && (tokens[pos].type == TokenType::PLUS ||
                                   tokens[pos].type == TokenType::MINUS))
    {
      TokenType op = tokens[pos].type;
      pos++; // consume operator
      auto right = parseMultiplicativeExpression(tokens, pos);
      if (right)
      {
        left = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right),
                                            tokens[pos - 2].line,
                                            tokens[pos - 2].column);
      }
    }

    return left;
  }

  std::unique_ptr<Expression>
  parseMultiplicativeExpression(const std::vector<Token> &tokens, size_t &pos)
  {
    auto left = parsePostfixExpression(tokens, pos);

    while (pos < tokens.size() && (tokens[pos].type == TokenType::MULTIPLY ||
                                   tokens[pos].type == TokenType::DIVIDE ||
                                   tokens[pos].type == TokenType::MODULO))
    {
      TokenType op = tokens[pos].type;
      pos++; // consume operator
      auto right = parsePostfixExpression(tokens, pos);
      if (right)
      {
        left = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right),
                                            tokens[pos - 2].line,
                                            tokens[pos - 2].column);
      }
    }

    return left;
  }

  std::unique_ptr<Expression>
  parsePostfixExpression(const std::vector<Token> &tokens, size_t &pos)
  {
    auto expr = parsePrimaryExpression(tokens, pos);

    // Handle postfix operations like function calls and member access
    while (pos < tokens.size())
    {
      if (tokens[pos].type == TokenType::LPAREN)
      {
        // Function call
        pos++; // consume '('
        std::vector<Expression::Ptr> arguments;

        // Parse arguments
        while (pos < tokens.size() && tokens[pos].type != TokenType::RPAREN)
        {
          auto arg = parseExpression(tokens, pos);
          if (arg)
          {
            arguments.push_back(std::move(arg));
          }

          // Check for comma
          if (pos < tokens.size() && tokens[pos].type == TokenType::COMMA)
          {
            pos++; // consume comma
          }
          else if (pos < tokens.size() && tokens[pos].type != TokenType::RPAREN)
          {
            // Expected comma or closing paren
            break;
          }
        }

        if (pos < tokens.size() && tokens[pos].type == TokenType::RPAREN)
        {
          size_t line = tokens[pos].line;
          size_t col = tokens[pos].column;
          pos++; // consume ')'
          expr = std::make_unique<CallExpr>(std::move(expr), std::move(arguments),
                                            line, col);
        }
      }
      else if (tokens[pos].type == TokenType::DOT)
      {
        // Member access
        pos++; // consume '.'
        if (pos < tokens.size() && tokens[pos].type == TokenType::IDENTIFIER)
        {
          std::string property = tokens[pos].text;
          size_t line = tokens[pos].line;
          size_t col = tokens[pos].column;
          pos++;
          expr = std::make_unique<MemberAccessExpr>(std::move(expr), property,
                                                    line, col);
        }
      }
      else if (tokens[pos].type == TokenType::LBRACKET)
      {
        // Array access
        pos++; // consume '['
        auto indexExpr = parseExpression(tokens, pos);

        if (pos < tokens.size() && tokens[pos].type == TokenType::RBRACKET)
        {
          pos++; // consume ']'
          expr = std::make_unique<ArrayAccessExpr>(std::move(expr), std::move(indexExpr),
                                                   tokens[pos - 1].line, tokens[pos - 1].column);
        }
      }
      else
      {
        break;
      }
    }

    return expr;
  }

  std::unique_ptr<Expression>
  parsePrimaryExpression(const std::vector<Token> &tokens, size_t &pos)
  {
    if (pos >= tokens.size())
      return nullptr;

    auto &token = tokens[pos];
    switch (token.type)
    {
    case TokenType::NUMBER:
    {
      // Parse as int or double
      char *end;
      double value = strtod(token.text.c_str(), &end);
      if (*end == 0)
      { // successfully parsed
        if (strchr(token.text.c_str(), '.') != nullptr)
        {
          pos++; // consume the token
          return std::make_unique<LiteralExpr>(value, token.line, token.column);
        }
        else
        {
          int intValue = static_cast<int>(value);
          pos++; // consume the token
          return std::make_unique<LiteralExpr>(intValue, token.line,
                                               token.column);
        }
      }
      pos++;
      return std::make_unique<LiteralExpr>(0, token.line,
                                           token.column); // fallback
    }
    case TokenType::STRING:
    {
      std::string strValue =
          token.text.substr(1, token.text.length() - 2); // remove quotes
      pos++;
      return std::make_unique<LiteralExpr>(strValue, token.line, token.column);
    }
    case TokenType::TRUE:
      pos++;
      return std::make_unique<LiteralExpr>(true, token.line, token.column);
    case TokenType::FALSE:
      pos++;
      return std::make_unique<LiteralExpr>(false, token.line, token.column);
    case TokenType::NULL_KEYWORD:
      pos++;
      return std::make_unique<LiteralExpr>(std::string("null"), token.line,
                                           token.column);
    case TokenType::IDENTIFIER:
    {
      std::string name = token.text;
      pos++;
      return std::make_unique<IdentifierExpr>(name, token.line, token.column);
    }
    case TokenType::LPAREN:
    {
      pos++; // consume '('
      auto expr = parseExpression(tokens, pos);
      if (pos < tokens.size() && tokens[pos].type == TokenType::RPAREN)
      {
        pos++; // consume ')'
      }
      return expr;
    }
    case TokenType::LBRACKET:
    {
      pos++; // consume '['
      std::vector<Expression::Ptr> elements;

      // Parse array elements separated by commas
      if (pos < tokens.size() && tokens[pos].type != TokenType::RBRACKET)
      {
        elements.push_back(parseExpression(tokens, pos));

        while (pos < tokens.size() && tokens[pos].type == TokenType::COMMA)
        {
          pos++; // consume comma
          if (pos < tokens.size() && tokens[pos].type != TokenType::RBRACKET)
          {
            elements.push_back(parseExpression(tokens, pos));
          }
        }
      }

      // Expect closing bracket
      if (pos < tokens.size() && tokens[pos].type == TokenType::RBRACKET)
      {
        pos++; // consume ']'
      }

      // Use the last token we consumed for position info, or default to 1,1
      size_t line = (pos > 0) ? tokens[pos - 1].line : 1;
      size_t col = (pos > 0) ? tokens[pos - 1].column : 1;
      return std::make_unique<ArrayLiteralExpr>(std::move(elements), line, col);
    }
    default:
      pos++;
      return nullptr;
    }
  }

  std::unique_ptr<Statement>
  parseFunctionDeclaration(const std::vector<Token> &tokens, size_t &pos)
  {
    // Skip the fun token
    if (pos < tokens.size() && tokens[pos].type == TokenType::FUN)
    {
      pos++;
    }

    // Expect function name
    std::string name = "anonymous"; // default name
    if (pos < tokens.size() && tokens[pos].type == TokenType::IDENTIFIER)
    {
      name = tokens[pos].text;
      pos++;
    }

    // Expect opening parenthesis for parameters
    std::vector<std::string> parameters;
    if (pos < tokens.size() && tokens[pos].type == TokenType::LPAREN)
    {
      pos++; // consume '('

      // Parse parameters
      while (pos < tokens.size() && tokens[pos].type != TokenType::RPAREN)
      {
        if (tokens[pos].type == TokenType::IDENTIFIER)
        {
          parameters.push_back(tokens[pos].text);
          pos++;

          // Expect comma or closing parenthesis
          if (pos < tokens.size() && tokens[pos].type == TokenType::COMMA)
          {
            pos++; // consume comma
          }
        }
        else
        {
          // Skip unexpected token
          pos++;
        }
      }

      if (pos < tokens.size() && tokens[pos].type == TokenType::RPAREN)
      {
        pos++; // consume ')'
      }
    }

    // Expect opening brace for function body
    Statement::Ptr body = nullptr;
    if (pos < tokens.size() && tokens[pos].type == TokenType::LBRACE)
    {
      // Parse block statement as function body
      size_t temp_pos = pos;
      body = parseBlockStatement(tokens, temp_pos);
      if (body)
      {
        pos = temp_pos; // update position if block was successfully parsed
      }
    }

    // Use the last token we consumed for position info, or default to 1,1
    size_t line = (pos > 0) ? tokens[pos - 1].line : 1;
    size_t col = (pos > 0) ? tokens[pos - 1].column : 1;
    return std::make_unique<FunctionDeclStmt>(
        name, std::move(parameters), std::move(body), line, col);
  }

  std::unique_ptr<Statement> parseIfStatement(const std::vector<Token> &tokens,
                                              size_t &pos)
  {
    // Skip the if token
    if (pos < tokens.size() && tokens[pos].type == TokenType::IF)
    {
      pos++;
    }

    // Parse the condition in parentheses
    Expression::Ptr condition = nullptr;
    if (pos < tokens.size() && tokens[pos].type == TokenType::LPAREN)
    {
      pos++; // consume '('
      condition = parseExpression(tokens, pos);
      if (pos < tokens.size() && tokens[pos].type == TokenType::RPAREN)
      {
        pos++; // consume ')'
      }
    }

    // Parse the then branch (could be a block or single statement)
    Statement::Ptr thenBranch = nullptr;
    if (pos < tokens.size() && tokens[pos].type == TokenType::LBRACE)
    {
      thenBranch = parseBlockStatement(tokens, pos);
    }
    else
    {
      thenBranch = parseStatement(tokens, pos);
    }

    // Check for else branch
    std::optional<Statement::Ptr> elseBranch = std::nullopt;
    if (pos < tokens.size() && tokens[pos].type == TokenType::ELSE)
    {
      pos++; // consume 'else'
      if (pos < tokens.size() && tokens[pos].type == TokenType::LBRACE)
      {
        elseBranch = parseBlockStatement(tokens, pos);
      }
      else
      {
        elseBranch = parseStatement(tokens, pos);
      }
    }

    // Use the last token we consumed for position info, or default to 1,1
    size_t line = (pos > 0) ? tokens[pos - 1].line : 1;
    size_t col = (pos > 0) ? tokens[pos - 1].column : 1;
    return std::make_unique<IfStmt>(std::move(condition), std::move(thenBranch),
                                    std::move(elseBranch), line, col);
  }

  std::unique_ptr<Statement>
  parseReturnStatement(const std::vector<Token> &tokens, size_t &pos)
  {
    // Skip the return token
    if (pos < tokens.size() && tokens[pos].type == TokenType::RETURN)
    {
      pos++;
    }

    // Parse the return expression (if present)
    Expression::Ptr returnValue = nullptr;
    if (pos < tokens.size() && tokens[pos].type != TokenType::SEMICOLON &&
        tokens[pos].type != TokenType::RBRACE &&
        tokens[pos].type != TokenType::EOF_TOKEN)
    {
      returnValue = parseExpression(tokens, pos);
    }

    // Use the last token we consumed for position info, or default to 1,1
    size_t line = (pos > 0) ? tokens[pos - 1].line : 1;
    size_t col = (pos > 0) ? tokens[pos - 1].column : 1;
    return std::make_unique<ReturnStmt>(
        std::move(returnValue), line, col);
  }

  std::unique_ptr<Statement> parseBlockStatement(const std::vector<Token> &tokens,
                                                 size_t &pos)
  {
    // Skip the opening brace
    if (pos < tokens.size() && tokens[pos].type == TokenType::LBRACE)
    {
      pos++;
    }

    // Parse statements until closing brace
    std::vector<Statement::Ptr> statements;
    while (pos < tokens.size() && tokens[pos].type != TokenType::RBRACE &&
           tokens[pos].type != TokenType::EOF_TOKEN)
    {
      auto stmt = parseStatement(tokens, pos);
      if (stmt)
      {
        statements.push_back(std::move(stmt));
      }
    }

    if (pos < tokens.size() && tokens[pos].type == TokenType::RBRACE)
    {
      pos++; // consume closing brace
    }

    // Use the last token we consumed for position info, or default to 1,1
    size_t line = (pos > 0) ? tokens[pos - 1].line : 1;
    size_t col = (pos > 0) ? tokens[pos - 1].column : 1;
    return std::make_unique<BlockStmt>(
        std::move(statements), line, col);
  }

  std::unique_ptr<Statement>
  parseExpressionStatement(const std::vector<Token> &tokens, size_t &pos)
  {
    // Parse an expression and wrap it in a statement
    auto expr = parseExpression(tokens, pos);
    if (expr)
    {
      // Wrap the expression in an ExpressionStmt
      // Use the last token we consumed for position info, or default to 1,1
      size_t line = (pos > 0) ? tokens[pos - 1].line : 1;
      size_t col = (pos > 0) ? tokens[pos - 1].column : 1;
      return std::make_unique<ExpressionStmt>(
          std::move(expr), line, col);
    }

    if (pos < tokens.size())
    {
      pos++; // consume one token to avoid infinite loop
    }
    return nullptr;
  }

} // namespace dotlin