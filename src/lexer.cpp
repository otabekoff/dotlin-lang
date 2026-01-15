// src/lexer.cpp
#include "dotlin/lexer.h"
#include <cctype>
// #include <sstream>

namespace dotlin {

std::vector<Token> tokenize(const std::string &src) {
  std::vector<Token> tokens;
  size_t line = 1;
  size_t column = 1;
  size_t i = 0;

  while (i < src.length()) {
    char ch = src[i];

    if (std::isspace(static_cast<unsigned char>(ch))) {
      if (ch == '\n') {
        line++;
        column = 1;
      } else {
        column++;
      }
      i++;
    } else if (std::isdigit(static_cast<unsigned char>(ch))) {
      size_t start = i;
      while (i < src.length() &&
             std::isdigit(static_cast<unsigned char>(src[i]))) {
        i++;
      }
      std::string num = src.substr(start, i - start);
      tokens.emplace_back(TokenType::NUMBER, num, line, column);
      column += num.length();
    } else if (ch == '"') {
      size_t start = i;
      i++; // Skip opening quote
      while (i < src.length() && src[i] != '"') {
        if (src[i] == '\n') {
          line++;
          column = 1;
        } else {
          column++;
        }
        i++;
      }
      if (i < src.length()) {
        i++; // Skip closing quote
      }
      std::string str = src.substr(start, i - start);
      tokens.emplace_back(TokenType::STRING, str, line, column);
      column += str.length();
    } else if (std::isalpha(static_cast<unsigned char>(ch)) || ch == '_') {
      size_t start = i;
      while (
          i < src.length() &&
          (std::isalnum(static_cast<unsigned char>(src[i])) || src[i] == '_')) {
        i++;
      }

      std::string identifier = src.substr(start, i - start);

      // Check if it's a keyword
      TokenType type = TokenType::IDENTIFIER;
      if (identifier == "fun")
        type = TokenType::FUN;
      else if (identifier == "val")
        type = TokenType::VAL;
      else if (identifier == "var")
        type = TokenType::VAR;
      else if (identifier == "class")
        type = TokenType::CLASS;
      else if (identifier == "if")
        type = TokenType::IF;
      else if (identifier == "else")
        type = TokenType::ELSE;
      else if (identifier == "true")
        type = TokenType::TRUE;
      else if (identifier == "false")
        type = TokenType::FALSE;
      else if (identifier == "null")
        type = TokenType::NULL_KEYWORD;
      else if (identifier == "return")
        type = TokenType::RETURN;
      else if (identifier == "for")
        type = TokenType::FOR;
      else if (identifier == "while")
        type = TokenType::WHILE;
      else if (identifier == "when")
        type = TokenType::WHEN;
      else if (identifier == "interface")
        type = TokenType::INTERFACE;
      else if (identifier == "object")
        type = TokenType::OBJECT;
      else if (identifier == "data")
        type = TokenType::DATA;
      else if (identifier == "sealed")
        type = TokenType::SEALED;
      else if (identifier == "abstract")
        type = TokenType::ABSTRACT;
      else if (identifier == "open")
        type = TokenType::OPEN;
      else if (identifier == "override")
        type = TokenType::OVERRIDE;
      else if (identifier == "try")
        type = TokenType::TRY;
      else if (identifier == "catch")
        type = TokenType::CATCH;
      else if (identifier == "finally")
        type = TokenType::FINALLY;
      else if (identifier == "break")
        type = TokenType::BREAK;
      else if (identifier == "continue")
        type = TokenType::CONTINUE;
      else if (identifier == "by")
        type = TokenType::BY;
      else if (identifier == "init")
        type = TokenType::INIT;
      else if (identifier == "import")
        type = TokenType::IMPORT;
      else if (identifier == "package")
        type = TokenType::PACKAGE;
      else if (identifier == "constructor")
        type = TokenType::CONSTRUCTOR;
      else if (identifier == "enum")
        type = TokenType::ENUM;
      else if (identifier == "super")
        type = TokenType::SUPER;
      else if (identifier == "this")
        type = TokenType::THIS;
      else if (identifier == "is")
        type = TokenType::IS;
      else if (identifier == "in")
        type = TokenType::IN;
      else if (identifier == "out")
        type = TokenType::OUT;

      tokens.emplace_back(type, identifier, line, column);
      column += identifier.length();
    } else {
      // Handle operators and delimiters
      TokenType type;
      std::string text(1, ch);
      size_t next_column = column + 1;

      switch (ch) {
      case '+':
        if (i + 1 < src.length() && src[i + 1] == '+') {
          text = "++";
          type = TokenType::INCREMENT;
          i++; // Skip next character
          next_column++;
        } else if (i + 1 < src.length() && src[i + 1] == '=') {
          text = "+=";
          type = TokenType::PLUS_ASSIGN;
          i++; // Skip next character
          next_column++;
        } else {
          type = TokenType::PLUS;
        }
        break;
      case '-':
        if (i + 1 < src.length() && src[i + 1] == '-') {
          text = "--";
          type = TokenType::DECREMENT;
          i++; // Skip next character
          next_column++;
        } else if (i + 1 < src.length() && src[i + 1] == '>') {
          text = "->";
          type = TokenType::ARROW;
          i++; // Skip next character
          next_column++;
        } else if (i + 1 < src.length() && src[i + 1] == '=') {
          text = "-=";
          type = TokenType::MINUS_ASSIGN;
          i++; // Skip next character
          next_column++;
        } else {
          type = TokenType::MINUS;
        }
        break;
      case '*':
        if (i + 1 < src.length() && src[i + 1] == '=') {
          text = "*=";
          type = TokenType::MULTIPLY_ASSIGN;
          i++; // Skip next character
          next_column++;
        } else {
          type = TokenType::MULTIPLY;
        }
        break;
      case '%':
        if (i + 1 < src.length() && src[i + 1] == '=') {
          text = "%=";
          type = TokenType::MODULO_ASSIGN;
          i++; // Skip next character
          next_column++;
        } else {
          type = TokenType::MODULO;
        }
        break;
      case '/':
        if (i + 1 < src.length() && src[i + 1] == '=') {
          text = "/=";
          type = TokenType::DIVIDE_ASSIGN;
          i++; // Skip next character
          next_column++;
        } else {
          type = TokenType::DIVIDE;
        }
        break;
      case '=':
        if (i + 1 < src.length() && src[i + 1] == '=') {
          text = "==";
          type = TokenType::EQUAL;
          i++; // Skip next character
          next_column++;
        } else {
          type = TokenType::ASSIGN;
        }
        break;
      case '!':
        if (i + 1 < src.length() && src[i + 1] == '=') {
          text = "!=";
          type = TokenType::NOT_EQUAL;
          i++; // Skip next character
          next_column++;
        } else {
          type = TokenType::NOT;
        }
        break;
      case '<':
        if (i + 1 < src.length() && src[i + 1] == '=') {
          text = "<=";
          type = TokenType::LESS_EQUAL;
          i++; // Skip next character
          next_column++;
        } else {
          type = TokenType::LESS;
        }
        break;
      case '>':
        if (i + 1 < src.length() && src[i + 1] == '=') {
          text = ">=";
          type = TokenType::GREATER_EQUAL;
          i++; // Skip next character
          next_column++;
        } else {
          type = TokenType::GREATER;
        }
        break;
      case '&':
        if (i + 1 < src.length() && src[i + 1] == '&') {
          text = "&&";
          type = TokenType::AND;
          i++; // Skip next character
          next_column++;
        } else {
          type = TokenType::UNKNOWN;
        }
        break;
      case '|':
        if (i + 1 < src.length() && src[i + 1] == '|') {
          text = "||";
          type = TokenType::OR;
          i++; // Skip next character
          next_column++;
        } else {
          type = TokenType::UNKNOWN;
        }
        break;
      case '(':
        type = TokenType::LPAREN;
        break;
      case ')':
        type = TokenType::RPAREN;
        break;
      case '{':
        type = TokenType::LBRACE;
        break;
      case '}':
        type = TokenType::RBRACE;
        break;
      case '[':
        type = TokenType::LBRACKET;
        break;
      case ']':
        type = TokenType::RBRACKET;
        break;
      case ',':
        type = TokenType::COMMA;
        break;
      case '.':
        if (i + 1 < src.length() && src[i + 1] == '.') {
          if (i + 2 < src.length() && src[i + 2] == '<') {
            text = "..<";
            type = TokenType::RANGE_UNTIL;
            i += 2; // Skip next two characters
            next_column += 2;
          } else {
            text = "..";
            type = TokenType::RANGE;
            i++; // Skip next character
            next_column++;
          }
        } else {
          type = TokenType::DOT;
        }
        break;
      case ';':
        type = TokenType::SEMICOLON;
        break;
      case ':':
        type = TokenType::COLON;
        break;
      case '?':
        if (i + 1 < src.length() && src[i + 1] == ':') {
          text = "?:";
          type = TokenType::ELVIS;
          i++; // Skip next character
          next_column++;
        } else {
          type = TokenType::UNKNOWN; // Or '?' token if needed
        }
        break;
      case '$':
        if (i + 1 < src.length() && src[i + 1] == '{') {
          text = "${";
          type = TokenType::DOLLAR_LBRACE;
          i++; // Skip next character
          next_column++;
        } else {
          type = TokenType::UNKNOWN;
        }
        break;
      default:
        type = TokenType::UNKNOWN;
        break;
      }

      // Additional defensive checks
      if (type == TokenType::PLUS_ASSIGN && text != "+=") {
        // Reset to UNKNOWN if PLUS_ASSIGN token doesn't have correct text
        type = TokenType::UNKNOWN;
      } else if (type == TokenType::INCREMENT && text != "++") {
        type = TokenType::UNKNOWN;
      } else if (type == TokenType::DECREMENT && text != "--") {
        type = TokenType::UNKNOWN;
      } else if (type == TokenType::MINUS_ASSIGN && text != "-=") {
        type = TokenType::UNKNOWN;
      } else if (type == TokenType::MULTIPLY_ASSIGN && text != "*=") {
        type = TokenType::UNKNOWN;
      } else if (type == TokenType::DIVIDE_ASSIGN && text != "/=") {
        type = TokenType::UNKNOWN;
      } else if (type == TokenType::MODULO_ASSIGN && text != "%=") {
        type = TokenType::UNKNOWN;
      } else if (type == TokenType::ELVIS && text != "?:") {
        type = TokenType::UNKNOWN;
      } else if (type == TokenType::RANGE && text != "..") {
        type = TokenType::UNKNOWN;
      } else if (type == TokenType::RANGE_UNTIL && text != "..<") {
        type = TokenType::UNKNOWN;
      } else if (type == TokenType::ARROW && text != "->") {
        type = TokenType::UNKNOWN;
      } else if (type == TokenType::DOLLAR_LBRACE && text != "${") {
        type = TokenType::UNKNOWN;
      }

      // Defensive check: ensure we're not creating an EOF token with non-empty
      // text
      if (type == TokenType::EOF_TOKEN && !text.empty()) {
        // This shouldn't happen - EOF tokens should have empty text
        // Reset to UNKNOWN to avoid confusion
        type = TokenType::UNKNOWN;
      }

      // Additional check: if text is empty for tokens that should have text,
      // mark as unknown
      if (text.empty() && type != TokenType::EOF_TOKEN) {
        type = TokenType::UNKNOWN;
      }

      tokens.emplace_back(type, text, line, column);
      column = next_column;
      i++;
    }
  }

  tokens.emplace_back(TokenType::EOF_TOKEN, "", line, column);

  // Filter out invalid tokens (workaround for potential lexer bugs)
  std::vector<Token> filtered_tokens;
  for (const auto &token : tokens) {
    // Skip tokens that have empty text but are not EOF
    if (token.text.empty() && token.type != TokenType::EOF_TOKEN) {
      continue; // Skip this invalid token
    }
    // Add valid tokens to filtered list
    filtered_tokens.push_back(token);
  }

  return filtered_tokens;
}

} // namespace dotlin