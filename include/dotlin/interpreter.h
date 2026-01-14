// Interpreter for Dotlin - Kotlin-like language implementation in C++
#pragma once
#include "dotlin/parser.h"
#include <any>
#include <string>
#include <unordered_map>
#include <vector>

namespace dotlin
{

  // Forward declaration
  struct ArrayValue;

  // Runtime value representation
  using Value = std::variant<int, double, bool, std::string, ArrayValue>;

  // Array element type enumeration
  enum class ArrayElementType
  {
    INT,
    DOUBLE,
    BOOL,
    STRING,
    MIXED,
    UNKNOWN
  };

  // Array value structure
  struct ArrayValue
  {
    std::vector<Value> elements;
    ArrayElementType elementType;

    ArrayValue() : elements(), elementType(ArrayElementType::UNKNOWN) {}
    ArrayValue(const std::vector<Value> &els) : elements(els), elementType(determineElementType(els)) {}
    ArrayValue(std::vector<Value> &&els) : elements(std::move(els)), elementType(determineElementType(elements)) {}

    // Constructor with explicit element type
    ArrayValue(const std::vector<Value> &els, ArrayElementType elemType)
        : elements(els), elementType(elemType) {}
    ArrayValue(std::vector<Value> &&els, ArrayElementType elemType)
        : elements(std::move(els)), elementType(elemType) {}

    // Determine the element type based on the elements in the array
    static ArrayElementType determineElementType(const std::vector<Value> &elems)
    {
      if (elems.empty())
        return ArrayElementType::UNKNOWN;

      ArrayElementType firstType = getValueType(elems[0]);
      for (size_t i = 1; i < elems.size(); ++i)
      {
        if (getValueType(elems[i]) != firstType)
        {
          return ArrayElementType::MIXED;
        }
      }
      return firstType;
    }

    // Get the type of a value
    static ArrayElementType getValueType(const Value &val)
    {
      if (std::holds_alternative<int>(val))
        return ArrayElementType::INT;
      else if (std::holds_alternative<double>(val))
        return ArrayElementType::DOUBLE;
      else if (std::holds_alternative<bool>(val))
        return ArrayElementType::BOOL;
      else if (std::holds_alternative<std::string>(val))
        return ArrayElementType::STRING;
      else
        return ArrayElementType::UNKNOWN;
    }

    // Get the size of the array
    size_t size() const { return elements.size(); }

    // Check if the array is empty
    bool empty() const { return elements.empty(); }

    // Get element at index
    Value get(size_t index) const
    {
      if (index < elements.size())
        return elements[index];
      else
        throw std::runtime_error("Array index out of bounds");
    }

    // Set element at index
    void set(size_t index, const Value &value)
    {
      if (index < elements.size())
        elements[index] = value;
      else
        throw std::runtime_error("Array index out of bounds");
    }

    // Add element to the end
    void push_back(const Value &value)
    {
      elements.push_back(value);
      // Update type if needed
      if (elements.size() == 1)
      {
        elementType = getValueType(value);
      }
      else if (getValueType(value) != elementType)
      {
        elementType = ArrayElementType::MIXED;
      }
    }

    // Remove last element
    void pop_back()
    {
      if (!elements.empty())
      {
        elements.pop_back();
        // Update type if needed
        if (elements.empty())
        {
          elementType = ArrayElementType::UNKNOWN;
        }
        else
        {
          // Recalculate type
          elementType = determineElementType(elements);
        }
      }
    }
  };

  // Helper function to create array value
  inline ArrayValue makeArray(const std::vector<Value> &elements)
  {
    return ArrayValue(elements);
  }

  // Helper function to create typed array value
  inline ArrayValue makeTypedArray(const std::vector<Value> &elements, ArrayElementType type)
  {
    return ArrayValue(elements, type);
  }

  // Helper function to convert ArrayValue to vector
  inline std::vector<Value> getArray(const Value &value)
  {
    if (std::holds_alternative<ArrayValue>(value))
    {
      return std::get<ArrayValue>(value).elements;
    }
    return std::vector<Value>();
  }

  // Equality operator for Value type
  inline bool operator==(const Value &lhs, const Value &rhs)
  {
    // Compare types first
    if (lhs.index() != rhs.index())
    {
      return false;
    }

    // Compare values based on type
    if (std::holds_alternative<int>(lhs))
    {
      return std::get<int>(lhs) == std::get<int>(rhs);
    }
    else if (std::holds_alternative<double>(lhs))
    {
      return std::get<double>(lhs) == std::get<double>(rhs);
    }
    else if (std::holds_alternative<bool>(lhs))
    {
      return std::get<bool>(lhs) == std::get<bool>(rhs);
    }
    else if (std::holds_alternative<std::string>(lhs))
    {
      return std::get<std::string>(lhs) == std::get<std::string>(rhs);
    }
    else if (std::holds_alternative<ArrayValue>(lhs))
    {
      // Compare array elements
      const auto &lhsArr = std::get<ArrayValue>(lhs).elements;
      const auto &rhsArr = std::get<ArrayValue>(rhs).elements;
      if (lhsArr.size() != rhsArr.size())
      {
        return false;
      }
      for (size_t i = 0; i < lhsArr.size(); ++i)
      {
        if (!(lhsArr[i] == rhsArr[i]))
        {
          return false;
        }
      }
      return true;
    }

    // Default case for unknown types
    return false;
  }

  // Environment for variable bindings
  struct Environment
  {
    std::unordered_map<std::string, Value> values;
    std::shared_ptr<Environment> enclosing = nullptr;
    bool is_block_scope = false; // Flag to identify block vs function scope

    Environment(std::shared_ptr<Environment> parent = nullptr, bool block_scope = false) : enclosing(parent), is_block_scope(block_scope) {}

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
    void visit(ArrayLiteralExpr &node);
    void visit(ExpressionStmt &node);
    void visit(VariableDeclStmt &node);
    void visit(FunctionDeclStmt &node);
    void visit(BlockStmt &node);
    void visit(ReturnStmt &node);
    void visit(IfStmt &node);
    void visit(WhileStmt &node);
    void visit(ForStmt &node);
    void visit(WhenStmt &node);
    void visit(TryStmt &node);

  private:
    std::shared_ptr<Environment> globals;
    std::shared_ptr<Environment> environment;
    std::shared_ptr<Environment> functionEnvironment; // Keep track of the function-level environment
    bool hasMainFunction;
    Statement::Ptr mainFunctionStmt;                // Store reference to main function if found
    std::vector<std::string> commandLineArgs;       // Store command-line arguments
    int evaluationDepth = 0;                        // Track evaluation depth to prevent infinite recursion
    static constexpr int MAX_EVALUATION_DEPTH = 50; // Maximum allowed evaluation depth

    Value evaluate(Expression &expr);
    void execute(Statement &stmt);
    Value executeBlock(const std::vector<Statement::Ptr> &statements,
                       std::shared_ptr<Environment> env);
    std::string valueToString(const Value &value);
    std::shared_ptr<Type> getTypeOfValue(const Value &value);
    std::string typeToString(const std::shared_ptr<Type> &type);

    // Type inference methods
    void performTypeInference(Program &program);
    void performTypeInferenceOnStatement(Statement &stmt, class TypeChecker &typeChecker);
    void performTypeInferenceOnExpression(Expression &expr, class TypeChecker &typeChecker);
  };

  Value interpret(const Program &program);
  Value interpret(const Program &program, const std::vector<std::string> &args);

} // namespace dotlin