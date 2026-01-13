// Lexer for Dotlin - Kotlin-like language implementation in C++
#pragma once
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace dotlin {

enum class TokenType {
  // Keywords
  FUN,
  VAL,
  VAR,
  CLASS,
  INTERFACE,
  OBJECT,
  IF,
  ELSE,
  WHILE,
  FOR,
  WHEN,
  TRY,
  CATCH,
  FINALLY,
  RETURN,
  BREAK,
  CONTINUE,
  NULL_KEYWORD,
  TRUE,
  FALSE,
  DATA,
  SEALED,
  ABSTRACT,
  OPEN,
  OVERRIDE,
  BY,
  INIT,
  IMPORT,
  PACKAGE,
  CONSTRUCTOR,
  ENUM,
  SUPER,
  THIS,
  IS,
  IN,
  OUT,

  // Literals
  IDENTIFIER,
  NUMBER,
  STRING,
  CHAR,

  // Operators
  PLUS,
  MINUS,
  MULTIPLY,
  DIVIDE,
  MODULO,
  ASSIGN,
  EQUAL,
  NOT_EQUAL,
  LESS,
  GREATER,
  LESS_EQUAL,
  GREATER_EQUAL,
  AND,
  OR,
  NOT,
  INCREMENT,
  DECREMENT,
  ELVIS,
  RANGE,
  RANGE_UNTIL,
  PLUS_ASSIGN,
  MINUS_ASSIGN,
  MULTIPLY_ASSIGN,
  DIVIDE_ASSIGN,
  MODULO_ASSIGN,

  // Delimiters
  LPAREN,
  RPAREN,
  LBRACE,
  RBRACE,
  LBRACKET,
  RBRACKET,
  COMMA,
  DOT,
  SEMICOLON,
  COLON,
  ARROW,

  // Special
  EOF_TOKEN,
  WHITESPACE,
  COMMENT,
  UNKNOWN
};

struct Token {
  TokenType type;
  std::string text;
  size_t line;
  size_t column;

  Token(TokenType t, std::string txt, size_t l, size_t c)
      : type(t), text(std::move(txt)), line(l), column(c) {}
};

std::vector<Token> tokenize(const std::string &src);

} // namespace dotlin
