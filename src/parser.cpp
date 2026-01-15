// src/parser.cpp
#include "dotlin/parser.h"
#include <cstring>
// #include <iostream>
// #include <stdexcept>

namespace dotlin {

// Forward declarations
std::unique_ptr<Expression>
parseStringInterpolation(const std::string &strValue, size_t line,
                         size_t column);

// Parser implementation for Dotlin
Program parse(const std::vector<Token> &tokens) {
  Program program;
  size_t pos = 0;

  while (pos < tokens.size() && tokens[pos].type != TokenType::EOF_TOKEN) {
    auto stmt = parseStatement(tokens, pos);
    if (stmt) {
      program.statements.push_back(std::move(stmt));
    } else {
      // If we couldn't parse a statement, advance the position to avoid
      // infinite loop
      pos++;
    }
  }

  return program;
}

std::unique_ptr<Statement> parseStatement(const std::vector<Token> &tokens,
                                          size_t &pos) {
  if (pos >= tokens.size())
    return nullptr;

  switch (tokens[pos].type) {
  case TokenType::VAL:
  case TokenType::VAR:
    return parseVariableDeclaration(tokens, pos);
  case TokenType::FUN:
    return parseFunctionDeclaration(tokens, pos);
  case TokenType::IF:
    return parseIfStatement(tokens, pos);
  case TokenType::WHILE:
    return parseWhileStatement(tokens, pos);
  case TokenType::FOR:
    return parseForStatement(tokens, pos);
  case TokenType::WHEN:
    return parseWhenStatement(tokens, pos);
  case TokenType::TRY:
    return parseTryStatement(tokens, pos);
  case TokenType::CLASS:
    return parseClassDeclaration(tokens, pos);
  case TokenType::RETURN:
    return parseReturnStatement(tokens, pos);
  case TokenType::LBRACE:
    return parseBlockStatement(tokens, pos);
  default:
    return parseExpressionStatement(tokens, pos);
  }
}

std::unique_ptr<Statement>
parseVariableDeclaration(const std::vector<Token> &tokens, size_t &pos) {
  bool isVal = tokens[pos].type == TokenType::VAL;
  pos++; // consume val/var token

  if (pos >= tokens.size() || tokens[pos].type != TokenType::IDENTIFIER) {
    // Error handling would go here
    return nullptr;
  }

  std::string name = tokens[pos].text;
  pos++; // consume identifier

  // Check for type annotation (after identifier, before assignment)
  std::optional<std::shared_ptr<dotlin::Type>> typeAnnotation = std::nullopt;
  if (pos < tokens.size() && tokens[pos].type == TokenType::COLON) {
    pos++; // consume colon
    if (pos < tokens.size() && tokens[pos].type == TokenType::IDENTIFIER) {
      // Parse type name
      std::string typeName = tokens[pos].text;
      pos++; // consume type name

      // Map type name to TypeKind
      dotlin::TypeKind kind = dotlin::TypeKind::UNKNOWN;
      std::shared_ptr<dotlin::Type> elementType = nullptr;
      std::vector<std::shared_ptr<dotlin::Type>> genericTypes;

      if (typeName == "Int")
        kind = dotlin::TypeKind::INT;
      else if (typeName == "Double")
        kind = dotlin::TypeKind::DOUBLE;
      else if (typeName == "Boolean" || typeName == "Bool")
        kind = dotlin::TypeKind::BOOL;
      else if (typeName == "String")
        kind = dotlin::TypeKind::STRING;
      else if (typeName == "Array") {
        kind = dotlin::TypeKind::ARRAY;
        // Check for generic type parameter in angle brackets
        if (pos < tokens.size() && tokens[pos].type == TokenType::LESS) {
          pos++; // consume '<'
          if (pos < tokens.size() &&
              tokens[pos].type == TokenType::IDENTIFIER) {
            std::string genericTypeName = tokens[pos].text;
            pos++; // consume generic type name

            // Map generic type name to TypeKind
            dotlin::TypeKind genericKind = dotlin::TypeKind::UNKNOWN;
            if (genericTypeName == "Int")
              genericKind = dotlin::TypeKind::INT;
            else if (genericTypeName == "Double")
              genericKind = dotlin::TypeKind::DOUBLE;
            else if (genericTypeName == "Boolean" || genericTypeName == "Bool")
              genericKind = dotlin::TypeKind::BOOL;
            else if (genericTypeName == "String")
              genericKind = dotlin::TypeKind::STRING;
            else if (genericTypeName == "Array")
              genericKind = dotlin::TypeKind::ARRAY;
            else if (genericTypeName == "Unit" || genericTypeName == "Void")
              genericKind = dotlin::TypeKind::VOID;

            auto genericType = std::make_shared<dotlin::Type>(genericKind);
            genericTypes.push_back(genericType);

            // Check for closing '>'
            if (pos < tokens.size() && tokens[pos].type == TokenType::GREATER) {
              pos++; // consume '>'
            }
          }
        }
      } else if (typeName == "Unit" || typeName == "Void")
        kind = dotlin::TypeKind::VOID;

      if (!genericTypes.empty()) {
        typeAnnotation = std::make_shared<dotlin::Type>(
            kind, elementType, std::move(genericTypes));
      } else {
        typeAnnotation = std::make_shared<dotlin::Type>(kind, elementType);
      }
    }
  }

  std::optional<Expression::Ptr> initializer = std::nullopt;
  if (pos < tokens.size() && tokens[pos].type == TokenType::ASSIGN) {
    pos++; // consume assignment token
    auto expr = parseExpression(tokens, pos);
    if (expr) {
      initializer = std::move(expr);
    }
  }

  // Use the last token we consumed for position info, or default to 1,1
  size_t line = (pos > 0) ? tokens[pos - 1].line : 1;
  size_t col = (pos > 0) ? tokens[pos - 1].column : 1;
  return std::make_unique<VariableDeclStmt>(isVal, name, typeAnnotation,
                                            std::move(initializer), line, col);
}

std::unique_ptr<Expression> parseExpression(const std::vector<Token> &tokens,
                                            size_t &pos) {
  return parseAssignmentExpression(tokens, pos);
}

std::unique_ptr<Expression>
parseAssignmentExpression(const std::vector<Token> &tokens, size_t &pos) {
  auto left = parseComparisonExpression(tokens, pos);

  // Check for assignment operator
  if (pos < tokens.size() && tokens[pos].type == TokenType::ASSIGN) {
    TokenType op = tokens[pos].type;
    pos++; // consume assignment token
    auto right = parseAssignmentExpression(tokens, pos); // right-associative
    if (right) {
      left = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right),
                                          tokens[pos - 2].line,
                                          tokens[pos - 2].column);
    }
  }

  return left;
}

std::unique_ptr<Expression>
parseComparisonExpression(const std::vector<Token> &tokens, size_t &pos) {
  auto left = parseAdditiveExpression(tokens, pos);

  while (pos < tokens.size() &&
         (tokens[pos].type == TokenType::EQUAL ||
          tokens[pos].type == TokenType::NOT_EQUAL ||
          tokens[pos].type == TokenType::LESS ||
          tokens[pos].type == TokenType::LESS_EQUAL ||
          tokens[pos].type == TokenType::GREATER ||
          tokens[pos].type == TokenType::GREATER_EQUAL)) {
    TokenType op = tokens[pos].type;
    pos++; // consume operator
    auto right = parseAdditiveExpression(tokens, pos);
    if (right) {
      left = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right),
                                          tokens[pos - 2].line,
                                          tokens[pos - 2].column);
    }
  }

  return left;
}

std::unique_ptr<Expression>
parseAdditiveExpression(const std::vector<Token> &tokens, size_t &pos) {
  auto left = parseMultiplicativeExpression(tokens, pos);

  while (pos < tokens.size() && (tokens[pos].type == TokenType::PLUS ||
                                 tokens[pos].type == TokenType::MINUS)) {
    TokenType op = tokens[pos].type;
    pos++; // consume operator
    auto right = parseMultiplicativeExpression(tokens, pos);
    if (right) {
      left = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right),
                                          tokens[pos - 2].line,
                                          tokens[pos - 2].column);
    }
  }

  return left;
}

std::unique_ptr<Expression>
parseMultiplicativeExpression(const std::vector<Token> &tokens, size_t &pos) {
  auto left = parsePostfixExpression(tokens, pos);

  while (pos < tokens.size() && (tokens[pos].type == TokenType::MULTIPLY ||
                                 tokens[pos].type == TokenType::DIVIDE ||
                                 tokens[pos].type == TokenType::MODULO)) {
    TokenType op = tokens[pos].type;
    pos++; // consume operator
    auto right = parsePostfixExpression(tokens, pos);
    if (right) {
      left = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right),
                                          tokens[pos - 2].line,
                                          tokens[pos - 2].column);
    }
  }

  return left;
}

std::unique_ptr<Expression>
parsePostfixExpression(const std::vector<Token> &tokens, size_t &pos) {
  auto expr = parsePrimaryExpression(tokens, pos);
  if (!expr)
    return nullptr;

  // Handle postfix operations like function calls and member access
  while (pos < tokens.size()) {
    if (tokens[pos].type == TokenType::LPAREN) {
      // Function call
      pos++; // consume '('
      std::vector<Expression::Ptr> arguments;

      // Parse arguments
      while (pos < tokens.size() && tokens[pos].type != TokenType::RPAREN) {
        auto arg = parseExpression(tokens, pos);
        if (arg) {
          arguments.push_back(std::move(arg));
        }

        // Check for comma
        if (pos < tokens.size() && tokens[pos].type == TokenType::COMMA) {
          pos++; // consume comma
        } else if (pos < tokens.size() &&
                   tokens[pos].type != TokenType::RPAREN) {
          // Expected comma or closing paren
          break;
        }
      }

      if (pos < tokens.size() && tokens[pos].type == TokenType::RPAREN) {
        size_t line = tokens[pos].line;
        size_t col = tokens[pos].column;
        pos++; // consume ')'

        auto callee = std::move(expr);
        expr = std::make_unique<CallExpr>(std::move(callee),
                                          std::move(arguments), line, col);
      }
    } else if (tokens[pos].type == TokenType::DOT) {
      // Member access
      pos++; // consume '.'
      if (pos < tokens.size() && (tokens[pos].type == TokenType::IDENTIFIER ||
                                  tokens[pos].type == TokenType::INIT)) {
        std::string property = tokens[pos].text;
        size_t line = tokens[pos].line;
        size_t col = tokens[pos].column;
        pos++;

        expr = std::make_unique<MemberAccessExpr>(std::move(expr), property,
                                                  line, col);
      }
    } else if (tokens[pos].type == TokenType::LBRACKET) {
      // Array access
      pos++; // consume '['
      auto indexExpr = parseExpression(tokens, pos);

      if (pos < tokens.size() && tokens[pos].type == TokenType::RBRACKET) {
        pos++; // consume ']'
        expr = std::make_unique<ArrayAccessExpr>(
            std::move(expr), std::move(indexExpr), tokens[pos - 1].line,
            tokens[pos - 1].column);
      }
    } else {
      break;
    }
  }

  return expr;
}

std::unique_ptr<Expression>
parseLambdaExpression(const std::vector<Token> &tokens, size_t &pos) {
  if (pos >= tokens.size() || tokens[pos].type != TokenType::LBRACE)
    return nullptr;

  // Consume the opening brace
  size_t lambdaLine = tokens[pos].line;
  size_t lambdaCol = tokens[pos].column;
  pos++;

  // Parse parameters if present (before arrow)
  std::vector<dotlin::FunctionParameter> parameters;

  // Check if the first token after '{' is an identifier (parameter) or '->' (no
  // parameters)
  if (pos < tokens.size() && tokens[pos].type == TokenType::ARROW) {
    // No parameters, just '->'
    pos++; // consume '->'
  } else if (pos < tokens.size() && tokens[pos].type == TokenType::IDENTIFIER) {
    // Parse parameters
    while (pos < tokens.size()) {
      if (tokens[pos].type == TokenType::IDENTIFIER) {
        std::string paramName = tokens[pos].text;
        pos++;

        // Check for parameter type annotation
        std::optional<std::shared_ptr<dotlin::Type>> paramType = std::nullopt;
        if (pos < tokens.size() && tokens[pos].type == TokenType::COLON) {
          pos++; // consume colon
          if (pos < tokens.size() &&
              tokens[pos].type == TokenType::IDENTIFIER) {
            // Parse parameter type name
            std::string typeName = tokens[pos].text;
            pos++; // consume type name

            // Map type name to TypeKind
            dotlin::TypeKind kind = dotlin::TypeKind::UNKNOWN;
            if (typeName == "Int")
              kind = dotlin::TypeKind::INT;
            else if (typeName == "Double")
              kind = dotlin::TypeKind::DOUBLE;
            else if (typeName == "Boolean" || typeName == "Bool")
              kind = dotlin::TypeKind::BOOL;
            else if (typeName == "String")
              kind = dotlin::TypeKind::STRING;
            else if (typeName == "Array")
              kind = dotlin::TypeKind::ARRAY;
            else if (typeName == "Unit" || typeName == "Void")
              kind = dotlin::TypeKind::VOID;

            paramType = std::make_shared<dotlin::Type>(kind);
          }
        }

        parameters.push_back(dotlin::FunctionParameter(paramName, paramType));

        // Check for comma or arrow
        if (pos < tokens.size() && tokens[pos].type == TokenType::COMMA) {
          pos++; // consume comma
          continue;
        } else if (pos < tokens.size() &&
                   tokens[pos].type == TokenType::ARROW) {
          pos++; // consume arrow
          break;
        } else {
          // Unexpected token
          break;
        }
      } else if (pos < tokens.size() && tokens[pos].type == TokenType::ARROW) {
        pos++; // consume arrow
        break;
      } else {
        // Unexpected token
        break;
      }
    }
  } else {
    // Just consume the lambda body without parameters
    // This is an error case, but we'll handle it by treating everything until
    // '}' as the body
  }

  // Parse the lambda body (everything until closing brace)
  // For now, we'll create a simple block with expressions/statements
  std::vector<Statement::Ptr> bodyStatements;

  // Parse statements until we hit the closing brace
  while (pos < tokens.size() && tokens[pos].type != TokenType::RBRACE) {
    // Try to parse an expression statement
    auto expr = parseExpression(tokens, pos);
    if (expr) {
      // Add the expression as a statement
      bodyStatements.push_back(std::make_unique<ExpressionStmt>(
          std::move(expr), tokens[pos > 0 ? pos - 1 : 0].line,
          tokens[pos > 0 ? pos - 1 : 0].column));

      // Skip semicolon if present
      if (pos < tokens.size() && tokens[pos].type == TokenType::SEMICOLON) {
        pos++;
      }
      continue;
    }

    // If we couldn't parse an expression, try parsing other statements
    // For now, we'll just break if we encounter a closing brace or EOF
    if (pos >= tokens.size() || tokens[pos].type == TokenType::RBRACE ||
        tokens[pos].type == TokenType::EOF_TOKEN) {
      break;
    }

    // Skip unrecognized token to avoid infinite loop
    pos++;
  }

  // Create a block statement for the body
  auto body = std::make_unique<BlockStmt>(std::move(bodyStatements), lambdaLine,
                                          lambdaCol);

  // Consume the closing brace
  if (pos < tokens.size() && tokens[pos].type == TokenType::RBRACE) {
    pos++; // consume '}'
  }

  return std::make_unique<LambdaExpr>(std::move(parameters), std::move(body),
                                      lambdaLine, lambdaCol);
}

std::unique_ptr<Expression>
parsePrimaryExpression(const std::vector<Token> &tokens, size_t &pos) {
  if (pos >= tokens.size())
    return nullptr;

  auto &token = tokens[pos];
  switch (token.type) {
  case TokenType::NUMBER: {
    // Parse as int or double
    char *end;
    double value = strtod(token.text.c_str(), &end);
    if (*end == 0) { // successfully parsed
      if (strchr(token.text.c_str(), '.') != nullptr) {
        pos++; // consume the token
        return std::make_unique<LiteralExpr>(value, token.line, token.column);
      } else {
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
  case TokenType::STRING: {
    std::string strValue =
        token.text.substr(1, token.text.length() - 2); // remove quotes
    pos++;

    // Check if this string contains interpolation by looking for ${ pattern
    if (strValue.find("${") != std::string::npos) {
      // Parse string interpolation
      return parseStringInterpolation(strValue, token.line, token.column);
    } else {
      return std::make_unique<LiteralExpr>(strValue, token.line, token.column);
    }
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
  case TokenType::IDENTIFIER: {
    std::string name = token.text;
    pos++;
    return std::make_unique<IdentifierExpr>(name, token.line, token.column);
  }
  case TokenType::THIS: {
    pos++;
    return std::make_unique<IdentifierExpr>("this", token.line, token.column);
  }
  case TokenType::LBRACE: {
    // Check if this is a lambda expression
    // Lambda syntax: { param1, param2 -> body }
    return parseLambdaExpression(tokens, pos);
  }
  case TokenType::LPAREN: {
    pos++; // consume '('
    auto expr = parseExpression(tokens, pos);
    if (pos < tokens.size() && tokens[pos].type == TokenType::RPAREN) {
      pos++; // consume ')'
    }
    return expr;
  }
  case TokenType::LBRACKET: {
    pos++; // consume '['
    std::vector<Expression::Ptr> elements;

    // Parse array elements separated by commas
    if (pos < tokens.size() && tokens[pos].type != TokenType::RBRACKET) {
      elements.push_back(parseExpression(tokens, pos));

      while (pos < tokens.size() && tokens[pos].type == TokenType::COMMA) {
        pos++; // consume comma
        if (pos < tokens.size() && tokens[pos].type != TokenType::RBRACKET) {
          elements.push_back(parseExpression(tokens, pos));
        }
      }
    }

    // Expect closing bracket
    if (pos < tokens.size() && tokens[pos].type == TokenType::RBRACKET) {
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
parseFunctionDeclaration(const std::vector<Token> &tokens, size_t &pos) {
  // Skip the fun token
  if (pos < tokens.size() && tokens[pos].type == TokenType::FUN) {
    pos++;
  }

  // Expect function name
  std::string name = "anonymous"; // default name
  if (pos < tokens.size() && (tokens[pos].type == TokenType::IDENTIFIER ||
                              tokens[pos].type == TokenType::INIT)) {
    name = tokens[pos].text;
    pos++;
  }

  // Expect opening parenthesis for parameters
  std::vector<dotlin::FunctionParameter> parameters;
  if (pos < tokens.size() && tokens[pos].type == TokenType::LPAREN) {
    pos++; // consume '('

    // Parse parameters
    while (pos < tokens.size() && tokens[pos].type != TokenType::RPAREN) {
      if (tokens[pos].type == TokenType::IDENTIFIER) {
        std::string paramName = tokens[pos].text;
        pos++;

        // Check for parameter type annotation
        std::optional<std::shared_ptr<dotlin::Type>> paramType = std::nullopt;
        if (pos < tokens.size() && tokens[pos].type == TokenType::COLON) {
          pos++; // consume colon
          if (pos < tokens.size() &&
              tokens[pos].type == TokenType::IDENTIFIER) {
            // Parse parameter type name
            std::string typeName = tokens[pos].text;
            pos++; // consume type name

            // Map type name to TypeKind
            dotlin::TypeKind kind = dotlin::TypeKind::UNKNOWN;
            if (typeName == "Int")
              kind = dotlin::TypeKind::INT;
            else if (typeName == "Double")
              kind = dotlin::TypeKind::DOUBLE;
            else if (typeName == "Boolean" || typeName == "Bool")
              kind = dotlin::TypeKind::BOOL;
            else if (typeName == "String")
              kind = dotlin::TypeKind::STRING;
            else if (typeName == "Array")
              kind = dotlin::TypeKind::ARRAY;
            else if (typeName == "Unit" || typeName == "Void")
              kind = dotlin::TypeKind::VOID;

            paramType = std::make_shared<dotlin::Type>(kind);
          } else {
            // Unsupported/complex type annotation (e.g. function type). Treat
            // as unknown.
            paramType =
                std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
          }

          // Skip remaining tokens of complex type annotations until ',' or ')'
          // at top-level.
          int parenDepth = 0;
          while (pos < tokens.size()) {
            if (tokens[pos].type == TokenType::LPAREN) {
              parenDepth++;
              pos++;
              continue;
            }
            if (tokens[pos].type == TokenType::RPAREN) {
              if (parenDepth == 0) {
                break;
              }
              parenDepth--;
              pos++;
              continue;
            }
            if (parenDepth == 0 && tokens[pos].type == TokenType::COMMA) {
              break;
            }
            pos++;
          }
        }

        parameters.push_back(dotlin::FunctionParameter(paramName, paramType));

        // Expect comma or closing parenthesis
        if (pos < tokens.size() && tokens[pos].type == TokenType::COMMA) {
          pos++; // consume comma
        }
      } else {
        // Skip unexpected token
        pos++;
      }
    }

    if (pos < tokens.size() && tokens[pos].type == TokenType::RPAREN) {
      pos++; // consume ')'
    }
  }

  // Check for return type annotation
  std::optional<std::shared_ptr<dotlin::Type>> returnType = std::nullopt;
  if (pos < tokens.size() && tokens[pos].type == TokenType::COLON) {
    pos++; // consume colon
    if (pos < tokens.size() && tokens[pos].type == TokenType::IDENTIFIER) {
      // Parse return type name
      std::string typeName = tokens[pos].text;
      pos++; // consume type name

      // Map type name to TypeKind
      dotlin::TypeKind kind = dotlin::TypeKind::UNKNOWN;
      if (typeName == "Int")
        kind = dotlin::TypeKind::INT;
      else if (typeName == "Double")
        kind = dotlin::TypeKind::DOUBLE;
      else if (typeName == "Boolean" || typeName == "Bool")
        kind = dotlin::TypeKind::BOOL;
      else if (typeName == "String")
        kind = dotlin::TypeKind::STRING;
      else if (typeName == "Array")
        kind = dotlin::TypeKind::ARRAY;
      else if (typeName == "Unit" || typeName == "Void")
        kind = dotlin::TypeKind::VOID;

      returnType = std::make_shared<dotlin::Type>(kind);
    }
  }

  // Expect opening brace for function body
  Statement::Ptr body = nullptr;
  if (pos < tokens.size() && tokens[pos].type == TokenType::LBRACE) {
    // Parse block statement as function body
    size_t temp_pos = pos;
    body = parseBlockStatement(tokens, temp_pos);
    if (body) {
      pos = temp_pos; // update position if block was successfully parsed
    }
  }

  // Use the last token we consumed for position info, or default to 1,1
  size_t line = (pos > 0) ? tokens[pos - 1].line : 1;
  size_t col = (pos > 0) ? tokens[pos - 1].column : 1;
  return std::make_unique<FunctionDeclStmt>(
      name, std::move(parameters), std::move(body), returnType, line, col);
}

std::unique_ptr<Statement> parseIfStatement(const std::vector<Token> &tokens,
                                            size_t &pos) {
  // Skip the if token
  if (pos < tokens.size() && tokens[pos].type == TokenType::IF) {
    pos++;
  }

  // Parse the condition in parentheses
  Expression::Ptr condition = nullptr;
  if (pos < tokens.size() && tokens[pos].type == TokenType::LPAREN) {
    pos++; // consume '('
    condition = parseExpression(tokens, pos);
    if (pos < tokens.size() && tokens[pos].type == TokenType::RPAREN) {
      pos++; // consume ')'
    }
  }

  // Parse the then branch (could be a block or single statement)
  Statement::Ptr thenBranch = nullptr;
  if (pos < tokens.size() && tokens[pos].type == TokenType::LBRACE) {
    thenBranch = parseBlockStatement(tokens, pos);
  } else {
    thenBranch = parseStatement(tokens, pos);
  }

  // Check for else branch
  std::optional<Statement::Ptr> elseBranch = std::nullopt;
  if (pos < tokens.size() && tokens[pos].type == TokenType::ELSE) {
    pos++; // consume 'else'
    if (pos < tokens.size() && tokens[pos].type == TokenType::LBRACE) {
      elseBranch = parseBlockStatement(tokens, pos);
    } else {
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
parseReturnStatement(const std::vector<Token> &tokens, size_t &pos) {
  // Skip the return token
  if (pos < tokens.size() && tokens[pos].type == TokenType::RETURN) {
    pos++;
  }

  // Parse the return expression (if present)
  Expression::Ptr returnValue = nullptr;
  if (pos < tokens.size() && tokens[pos].type != TokenType::SEMICOLON &&
      tokens[pos].type != TokenType::RBRACE &&
      tokens[pos].type != TokenType::EOF_TOKEN) {
    returnValue = parseExpression(tokens, pos);
  }

  // Use the last token we consumed for position info, or default to 1,1
  size_t line = (pos > 0) ? tokens[pos - 1].line : 1;
  size_t col = (pos > 0) ? tokens[pos - 1].column : 1;
  return std::make_unique<ReturnStmt>(std::move(returnValue), line, col);
}

std::unique_ptr<Statement> parseBlockStatement(const std::vector<Token> &tokens,
                                               size_t &pos) {
  // Skip the opening brace
  if (pos < tokens.size() && tokens[pos].type == TokenType::LBRACE) {
    pos++;
  }

  // Parse statements until closing brace
  std::vector<Statement::Ptr> statements;
  while (pos < tokens.size() && tokens[pos].type != TokenType::RBRACE &&
         tokens[pos].type != TokenType::EOF_TOKEN) {
    auto stmt = parseStatement(tokens, pos);
    if (stmt) {
      statements.push_back(std::move(stmt));
    }
  }

  if (pos < tokens.size() && tokens[pos].type == TokenType::RBRACE) {
    pos++; // consume closing brace
  }

  // Use the last token we consumed for position info, or default to 1,1
  size_t line = (pos > 0) ? tokens[pos - 1].line : 1;
  size_t col = (pos > 0) ? tokens[pos - 1].column : 1;
  return std::make_unique<BlockStmt>(std::move(statements), line, col);
}

std::unique_ptr<Statement>
parseExpressionStatement(const std::vector<Token> &tokens, size_t &pos) {
  // Parse an expression and wrap it in a statement
  auto expr = parseExpression(tokens, pos);
  if (expr) {
    // Wrap the expression in an ExpressionStmt
    // Use the last token we consumed for position info, or default to 1,1
    size_t line = (pos > 0) ? tokens[pos - 1].line : 1;
    size_t col = (pos > 0) ? tokens[pos - 1].column : 1;
    return std::make_unique<ExpressionStmt>(std::move(expr), line, col);
  }

  if (pos < tokens.size()) {
    pos++; // consume one token to avoid infinite loop
  }
  return nullptr;
}

std::unique_ptr<Statement> parseWhileStatement(const std::vector<Token> &tokens,
                                               size_t &pos) {
  // Skip the 'while' token
  if (pos < tokens.size() && tokens[pos].type == TokenType::WHILE) {
    pos++;
  }

  // Expect opening parenthesis for condition
  if (pos < tokens.size() && tokens[pos].type == TokenType::LPAREN) {
    pos++; // consume '('
  } else {
    // Error: expected '(' after while
    return nullptr;
  }

  // Parse the condition expression
  auto condition = parseExpression(tokens, pos);

  // Expect closing parenthesis
  if (pos < tokens.size() && tokens[pos].type == TokenType::RPAREN) {
    pos++; // consume ')'
  } else {
    // Error: expected ')' after while condition
    return nullptr;
  }

  // Parse the body of the while loop
  std::unique_ptr<Statement> body = nullptr;
  if (pos < tokens.size() && tokens[pos].type == TokenType::LBRACE) {
    // Block statement
    body = parseBlockStatement(tokens, pos);
  } else {
    // Single statement
    body = parseStatement(tokens, pos);
  }

  // Use the last token we consumed for position info, or default to 1,1
  size_t line = (pos > 0) ? tokens[pos - 1].line : 1;
  size_t col = (pos > 0) ? tokens[pos - 1].column : 1;
  return std::make_unique<WhileStmt>(std::move(condition), std::move(body),
                                     line, col);
}

std::unique_ptr<Statement> parseForStatement(const std::vector<Token> &tokens,
                                             size_t &pos) {
  // Skip the 'for' token
  if (pos < tokens.size() && tokens[pos].type == TokenType::FOR) {
    pos++;
  }

  // Expect opening parenthesis
  if (pos < tokens.size() && tokens[pos].type == TokenType::LPAREN) {
    pos++; // consume '('
  } else {
    // Error: expected '(' after for
    return nullptr;
  }

  std::string variableName;
  // Parse the variable name
  if (pos + 1 < tokens.size() && tokens[pos].type == TokenType::IDENTIFIER &&
      tokens[pos + 1].type == TokenType::IN) {
    variableName = tokens[pos].text;
    pos += 2; // consume variable name and 'in'
  } else {
    // Error: expected 'identifier in' after '('
    return nullptr;
  }

  // Parse the iterable expression
  auto iterable = parseExpression(tokens, pos);

  // Expect closing parenthesis
  if (pos < tokens.size() && tokens[pos].type == TokenType::RPAREN) {
    pos++; // consume ')'
  } else {
    // Error: expected ')' after for clause
    return nullptr;
  }

  // Parse the body of the for loop
  std::unique_ptr<Statement> body = nullptr;
  if (pos < tokens.size() && tokens[pos].type == TokenType::LBRACE) {
    // Block statement
    body = parseBlockStatement(tokens, pos);
  } else {
    // Single statement
    body = parseStatement(tokens, pos);
  }

  // Use the last token we consumed for position info, or default to 1,1
  size_t line = (pos > 0) ? tokens[pos - 1].line : 1;
  size_t col = (pos > 0) ? tokens[pos - 1].column : 1;
  return std::make_unique<ForStmt>(variableName, std::move(iterable),
                                   std::move(body), line, col);
}

std::unique_ptr<Statement> parseTryStatement(const std::vector<Token> &tokens,
                                             size_t &pos) {
  // Skip the 'try' token
  if (pos < tokens.size() && tokens[pos].type == TokenType::TRY) {
    pos++;
  } else {
    return nullptr;
  }

  // Parse the try block (must be a block statement)
  Statement::Ptr tryBlock = nullptr;
  if (pos < tokens.size() && tokens[pos].type == TokenType::LBRACE) {
    tryBlock = parseBlockStatement(tokens, pos);
  } else {
    // Error: try block must be a block statement
    return nullptr;
  }

  // Expect 'catch' keyword
  if (pos < tokens.size() && tokens[pos].type == TokenType::CATCH) {
    pos++; // consume 'catch'
  } else {
    // Error: expected catch after try
    return nullptr;
  }

  // Expect opening parenthesis for exception variable
  std::string exceptionVar;
  if (pos < tokens.size() && tokens[pos].type == TokenType::LPAREN) {
    pos++; // consume '('

    if (pos < tokens.size() && tokens[pos].type == TokenType::IDENTIFIER) {
      exceptionVar = tokens[pos].text;
      pos++; // consume identifier
    } else {
      // Error: expected identifier in catch clause
      return nullptr;
    }

    if (pos < tokens.size() && tokens[pos].type == TokenType::RPAREN) {
      pos++; // consume ')'
    } else {
      // Error: expected ')' after exception variable
      return nullptr;
    }
  } else {
    // Error: expected '(' after catch
    return nullptr;
  }

  // Parse the catch block (must be a block statement)
  Statement::Ptr catchBlock = nullptr;
  if (pos < tokens.size() && tokens[pos].type == TokenType::LBRACE) {
    catchBlock = parseBlockStatement(tokens, pos);
  } else {
    // Error: catch block must be a block statement
    return nullptr;
  }

  // Check for optional finally block
  std::optional<Statement::Ptr> finallyBlock = std::nullopt;
  if (pos < tokens.size() && tokens[pos].type == TokenType::FINALLY) {
    pos++; // consume 'finally'

    if (pos < tokens.size() && tokens[pos].type == TokenType::LBRACE) {
      finallyBlock = parseBlockStatement(tokens, pos);
    } else {
      // Error: finally block must be a block statement
      return nullptr;
    }
  }

  // Use the last token we consumed for position info, or default to 1,1
  size_t line = (pos > 0) ? tokens[pos - 1].line : 1;
  size_t col = (pos > 0) ? tokens[pos - 1].column : 1;
  return std::make_unique<TryStmt>(std::move(tryBlock), exceptionVar,
                                   std::move(catchBlock),
                                   std::move(finallyBlock), line, col);
}

std::unique_ptr<Statement> parseWhenStatement(const std::vector<Token> &tokens,
                                              size_t &pos) {
  // Skip the 'when' token
  if (pos < tokens.size() && tokens[pos].type == TokenType::WHEN) {
    pos++;
  }

  // Expect opening parenthesis
  if (pos < tokens.size() && tokens[pos].type == TokenType::LPAREN) {
    pos++; // consume '('
  } else {
    // Error: expected '(' after when
    return nullptr;
  }

  // Parse the subject expression
  auto subject = parseExpression(tokens, pos);

  // Expect closing parenthesis
  if (pos < tokens.size() && tokens[pos].type == TokenType::RPAREN) {
    pos++; // consume ')'
  } else {
    // Error: expected ')' after when subject
    return nullptr;
  }

  // Expect opening brace for branches
  if (pos < tokens.size() && tokens[pos].type == TokenType::LBRACE) {
    pos++; // consume '{'
  } else {
    // Error: expected '{' after when clause
    return nullptr;
  }

  // Parse branches
  std::vector<std::pair<Expression::Ptr, Statement::Ptr>> branches;
  std::optional<Statement::Ptr> elseBranch = std::nullopt;

  while (pos < tokens.size() && tokens[pos].type != TokenType::RBRACE) {
    // Check for 'else' branch
    if (pos < tokens.size() && tokens[pos].type == TokenType::ELSE) {
      pos++; // consume 'else'

      if (pos < tokens.size() && tokens[pos].type == TokenType::ARROW) {
        pos++; // consume '->'

        // Parse the else branch statement
        Statement::Ptr elseStmt = nullptr;
        if (pos < tokens.size() && tokens[pos].type == TokenType::LBRACE) {
          elseStmt = parseBlockStatement(tokens, pos);
        } else {
          elseStmt = parseStatement(tokens, pos);
        }

        elseBranch = std::move(elseStmt);
      }
      break; // else should be the last branch
    } else {
      // Parse pattern -> statement
      auto pattern = parseExpression(tokens, pos);

      if (pos < tokens.size() && tokens[pos].type == TokenType::ARROW) {
        pos++; // consume '->'

        // Parse the statement for this branch
        Statement::Ptr branchStmt = nullptr;
        if (pos < tokens.size() && tokens[pos].type == TokenType::LBRACE) {
          branchStmt = parseBlockStatement(tokens, pos);
        } else {
          branchStmt = parseStatement(tokens, pos);
        }

        branches.push_back(
            std::make_pair(std::move(pattern), std::move(branchStmt)));
      } else {
        // Error: expected '->' after pattern
        break;
      }
    }

    // Skip potential semicolons or commas between branches
    while (pos < tokens.size() && (tokens[pos].type == TokenType::SEMICOLON ||
                                   tokens[pos].type == TokenType::COMMA)) {
      pos++;
    }
  }

  // Expect closing brace
  if (pos < tokens.size() && tokens[pos].type == TokenType::RBRACE) {
    pos++; // consume '}'
  } else {
    // Error: expected '}' after when branches
    return nullptr;
  }

  // Use the last token we consumed for position info, or default to 1,1
  size_t line = (pos > 0) ? tokens[pos - 1].line : 1;
  size_t col = (pos > 0) ? tokens[pos - 1].column : 1;
  return std::make_unique<WhenStmt>(std::move(subject), std::move(branches),
                                    std::move(elseBranch), line, col);
}

std::unique_ptr<Statement>
parseClassDeclaration(const std::vector<Token> &tokens, size_t &pos) {
  // Skip the 'class' token
  if (pos < tokens.size() && tokens[pos].type == TokenType::CLASS) {
    pos++;
  }

  // Expect class name
  std::string className = "Anonymous"; // default name
  if (pos < tokens.size() && tokens[pos].type == TokenType::IDENTIFIER) {
    className = tokens[pos].text;
    pos++;
  }

  // Check for inheritance (class Parent : SuperClass)
  std::optional<std::string> superClass = std::nullopt;
  if (pos < tokens.size() && tokens[pos].type == TokenType::COLON) {
    pos++; // consume ':'
    if (pos < tokens.size() && tokens[pos].type == TokenType::IDENTIFIER) {
      superClass = tokens[pos].text;
      pos++; // consume superclass name
    }
  }

  // Expect opening brace for class body
  std::vector<Statement::Ptr> members;
  if (pos < tokens.size() && tokens[pos].type == TokenType::LBRACE) {
    pos++; // consume '{'

    // Parse class members (fields, methods, constructors) until closing brace
    while (pos < tokens.size() && tokens[pos].type != TokenType::RBRACE &&
           tokens[pos].type != TokenType::EOF_TOKEN) {
      // Check if this is a constructor
      if (pos < tokens.size() && tokens[pos].type == TokenType::CONSTRUCTOR) {
        pos++; // consume 'constructor' token

        // Expect opening parenthesis for constructor parameters
        std::vector<dotlin::FunctionParameter> parameters;
        if (pos < tokens.size() && tokens[pos].type == TokenType::LPAREN) {
          pos++; // consume '('

          // Parse constructor parameters
          while (pos < tokens.size() && tokens[pos].type != TokenType::RPAREN) {
            if (tokens[pos].type == TokenType::IDENTIFIER) {
              std::string paramName = tokens[pos].text;
              pos++;

              // Check for parameter type annotation
              std::optional<std::shared_ptr<dotlin::Type>> paramType =
                  std::nullopt;
              if (pos < tokens.size() && tokens[pos].type == TokenType::COLON) {
                pos++; // consume colon
                if (pos < tokens.size() &&
                    tokens[pos].type == TokenType::IDENTIFIER) {
                  // Parse parameter type name
                  std::string typeName = tokens[pos].text;
                  pos++; // consume type name

                  // Map type name to TypeKind
                  dotlin::TypeKind kind = dotlin::TypeKind::UNKNOWN;
                  if (typeName == "Int")
                    kind = dotlin::TypeKind::INT;
                  else if (typeName == "Double")
                    kind = dotlin::TypeKind::DOUBLE;
                  else if (typeName == "Boolean" || typeName == "Bool")
                    kind = dotlin::TypeKind::BOOL;
                  else if (typeName == "String")
                    kind = dotlin::TypeKind::STRING;
                  else if (typeName == "Array")
                    kind = dotlin::TypeKind::ARRAY;
                  else if (typeName == "Unit" || typeName == "Void")
                    kind = dotlin::TypeKind::VOID;

                  paramType = std::make_shared<dotlin::Type>(kind);
                }
              }

              parameters.push_back(
                  dotlin::FunctionParameter(paramName, paramType));

              // Expect comma or closing parenthesis
              if (pos < tokens.size() && tokens[pos].type == TokenType::COMMA) {
                pos++; // consume comma
              }
            } else {
              // Skip unexpected token
              pos++;
            }
          }

          if (pos < tokens.size() && tokens[pos].type == TokenType::RPAREN) {
            pos++; // consume ')'
          }
        }

        // Expect opening brace for constructor body
        Statement::Ptr body = nullptr;
        if (pos < tokens.size() && tokens[pos].type == TokenType::LBRACE) {
          // Parse block statement as constructor body
          size_t temp_pos = pos;
          body = parseBlockStatement(tokens, temp_pos);
          if (body) {
            pos = temp_pos; // update position if block was successfully parsed
          }
        }

        // Create constructor declaration statement
        auto constructor = std::make_unique<ConstructorDeclStmt>(
            std::move(parameters), std::move(body), className,
            tokens[pos - 1].line, tokens[pos - 1].column);
        members.push_back(std::move(constructor));
      } else {
        // Parse other members (methods, fields)
        auto member = parseStatement(tokens, pos);
        if (member) {
          members.push_back(std::move(member));
        } else {
          // Skip token to avoid infinite loop
          pos++;
        }
      }
    }

    // Consume closing brace
    if (pos < tokens.size() && tokens[pos].type == TokenType::RBRACE) {
      pos++;
    }
  }

  // Use the last token we consumed for position info, or default to 1,1
  size_t line = (pos > 0) ? tokens[pos - 1].line : 1;
  size_t col = (pos > 0) ? tokens[pos - 1].column : 1;
  return std::make_unique<ClassDeclStmt>(className, std::move(members),
                                         superClass, line, col);
}

std::unique_ptr<Expression>
parseStringInterpolation(const std::string &strValue, size_t line,
                         size_t column) {
  std::vector<Expression::Ptr> parts;
  size_t currentPos = 0;

  while (currentPos < strValue.length()) {
    size_t interpolationStart = strValue.find("${", currentPos);

    if (interpolationStart == std::string::npos) {
      // No more interpolations, add rest as a string literal
      if (currentPos < strValue.length()) {
        std::string remaining = strValue.substr(currentPos);
        parts.push_back(std::make_unique<LiteralExpr>(remaining, line, column));
      }
      break;
    }

    // Add string part before interpolation
    if (interpolationStart > currentPos) {
      std::string stringPart =
          strValue.substr(currentPos, interpolationStart - currentPos);
      parts.push_back(std::make_unique<LiteralExpr>(stringPart, line, column));
    }

    // Find the closing brace
    size_t interpolationEnd = strValue.find('}', interpolationStart + 2);
    if (interpolationEnd == std::string::npos) {
      // Unclosed interpolation, treat as literal
      std::string remaining = strValue.substr(currentPos);
      parts.push_back(std::make_unique<LiteralExpr>(remaining, line, column));
      break;
    }

    // Extract the expression inside ${}
    std::string expressionStr = strValue.substr(
        interpolationStart + 2, interpolationEnd - interpolationStart - 2);

    // For complex expressions, we need to tokenize and parse the expression
    // Create a temporary token stream for the expression
    std::vector<Token> exprTokens;

    // Simple tokenization for basic expressions
    size_t exprPos = 0;
    while (exprPos < expressionStr.length()) {
      char ch = expressionStr[exprPos];

      if (std::isalpha(ch) || ch == '_') {
        // Identifier
        size_t start = exprPos;
        while (exprPos < expressionStr.length() &&
               (std::isalnum(expressionStr[exprPos]) ||
                expressionStr[exprPos] == '_')) {
          exprPos++;
        }
        std::string ident = expressionStr.substr(start, exprPos - start);
        exprTokens.emplace_back(TokenType::IDENTIFIER, ident, line, column);
      } else if (ch == '.') {
        // Member access operator
        exprTokens.emplace_back(TokenType::DOT, ".", line, column);
        exprPos++;
      } else if (ch == '[') {
        // Array access operator
        exprTokens.emplace_back(TokenType::LBRACKET, "[", line, column);
        exprPos++;
      } else if (ch == ']') {
        // Array access operator
        exprTokens.emplace_back(TokenType::RBRACKET, "]", line, column);
        exprPos++;
      } else if (ch == '(') {
        // Function call
        exprTokens.emplace_back(TokenType::LPAREN, "(", line, column);
        exprPos++;
      } else if (ch == ')') {
        // Function call
        exprTokens.emplace_back(TokenType::RPAREN, ")", line, column);
        exprPos++;
      } else if (ch == ',') {
        // Argument separator
        exprTokens.emplace_back(TokenType::COMMA, ",", line, column);
        exprPos++;
      } else if (std::isdigit(ch)) {
        // Number
        size_t start = exprPos;
        while (exprPos < expressionStr.length() &&
               std::isdigit(expressionStr[exprPos])) {
          exprPos++;
        }
        std::string num = expressionStr.substr(start, exprPos - start);
        exprTokens.emplace_back(TokenType::NUMBER, num, line, column);
      } else if (std::isspace(ch)) {
        // Skip whitespace
        exprPos++;
      } else {
        // Unknown character, skip for now
        exprPos++;
      }
    }

    // Parse the expression using our token stream
    size_t tokenPos = 0;
    try {
      auto expr = parseExpression(exprTokens, tokenPos);
      if (expr) {
        parts.push_back(std::move(expr));
      } else {
        // If parsing fails, treat as literal
        std::string literalStr = "${" + expressionStr + "}";
        parts.push_back(
            std::make_unique<LiteralExpr>(literalStr, line, column));
      }
    } catch (...) {
      // If parsing fails, treat as literal
      std::string literalStr = "${" + expressionStr + "}";
      parts.push_back(std::make_unique<LiteralExpr>(literalStr, line, column));
    }

    currentPos = interpolationEnd + 1;
  }

  return std::make_unique<StringInterpolationExpr>(std::move(parts), line,
                                                   column);
}

} // namespace dotlin