// Interpreter for Dotlin - Kotlin-like language implementation in C++
#pragma once
#include "dotlin/parser.h"
// #include <any>
// #include <functional>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace dotlin {

// Forward declarations
struct ArrayValue;
struct FunctionDef;
struct LambdaValue;
struct Environment;
struct Type;
struct ClassDefinition;

class DotlinError : public std::runtime_error {
public:
  std::string type;
  size_t line;
  size_t column;
  std::string source;
  std::vector<std::string> stackTrace;

  DotlinError(std::string t, std::string m, size_t l, size_t c,
              std::string s = "source.lin")
      : std::runtime_error(std::move(m)), type(std::move(t)), line(l),
        column(c), source(std::move(s)) {}

  void setStackTrace(std::vector<std::string> trace) {
    stackTrace = std::move(trace);
  }

  void setSource(std::string s) {
    if (source == "source.lin" || source == "") {
      source = std::move(s);
    }
  }

  std::string fullMessage() const {
    std::string msg = type + " Error at " + source + ":" +
                      std::to_string(line) + ":" + std::to_string(column) +
                      ": " + what();
    if (!stackTrace.empty()) {
      msg += "\nStack Trace:";
      for (const auto &frame : stackTrace) {
        msg += "\n  at " + frame;
      }
    }
    return msg;
  }
};

#include <cstdint>

// Runtime value representation
using Value = std::variant<int, int64_t, double, bool, std::string, ArrayValue,
                           std::shared_ptr<LambdaValue>,
                           std::shared_ptr<struct ClassInstance>,
                           std::shared_ptr<ClassDefinition>>;

// Class instance structure
struct ClassInstance {
  std::string className;
  std::unordered_map<std::string, Value> fields;
  std::shared_ptr<ClassDefinition> classDef;

  ClassInstance(const std::string &clsName,
                std::shared_ptr<ClassDefinition> def)
      : className(clsName), classDef(std::move(def)) {}
};

// Class definition structure
struct ClassDefinition {
  std::string name;
  std::vector<std::pair<std::string, std::shared_ptr<Type>>> fields;
  std::vector<std::shared_ptr<VariableDeclStmt>>
      fieldDecls; // To store field initializers
  std::vector<std::shared_ptr<FunctionDef>> constructors;
  std::vector<std::shared_ptr<FunctionDef>> methods;
  std::shared_ptr<ClassDefinition> superclass;

  ClassDefinition(const std::string &className)
      : name(className), superclass(nullptr) {}
};

// Structure to represent a function definition
struct FunctionDef {
  std::string name;
  std::vector<FunctionParameter> parameters;
  Statement::Ptr body;
  size_t paramHash; // Hash based on parameter types for overload resolution

  FunctionDef(std::string funcName, std::vector<FunctionParameter> params,
              std::shared_ptr<Statement> funcBody)
      : name(std::move(funcName)), parameters(std::move(params)),
        body(std::move(funcBody)), paramHash(calculateParamHash()) {}

  // Calculate a hash based on parameter types for overload resolution
  size_t calculateParamHash() const {
    size_t hash = 0;
    for (const auto &param : parameters) {
      if (param.typeAnnotation.has_value() && param.typeAnnotation.value()) {
        hash ^= static_cast<size_t>(param.typeAnnotation.value()->kind) +
                0x9e3779b9 + (hash << 6) + (hash >> 2);
      } else {
        // Use a special value for untyped parameters
        hash ^= static_cast<size_t>(TypeKind::UNKNOWN) + 0x9e3779b9 +
                (hash << 6) + (hash >> 2);
      }
    }
    return hash;
  }
};

// Forward declaration for LambdaExpr
struct LambdaExpr;

// Structure to represent a lambda expression value
struct LambdaValue {
  std::vector<FunctionParameter> parameters;
  Statement::Ptr body;
  std::shared_ptr<Environment> closure;
  const LambdaExpr
      *original_node; // Reference to original lambda expression in AST

  LambdaValue(std::vector<FunctionParameter> params, Statement::Ptr b,
              std::shared_ptr<Environment> env)
      : parameters(std::move(params)), body(std::move(b)),
        closure(std::move(env)), original_node(nullptr) {}

  LambdaValue(std::vector<FunctionParameter> params, Statement::Ptr b,
              std::shared_ptr<Environment> env, const LambdaExpr *node)
      : parameters(std::move(params)), body(std::move(b)),
        closure(std::move(env)), original_node(node) {}
};

// Array element type enumeration
enum class ArrayElementType { INT, DOUBLE, BOOL, STRING, MIXED, UNKNOWN };

// Array value structure
// Array value structure
struct ArrayValue {
  std::shared_ptr<std::vector<Value>> elements;
  ArrayElementType elementType;

  ArrayValue()
      : elements(std::make_shared<std::vector<Value>>()),
        elementType(ArrayElementType::UNKNOWN) {}

  ArrayValue(const std::vector<Value> &els)
      : elements(std::make_shared<std::vector<Value>>(els)),
        elementType(determineElementType(els)) {}

  ArrayValue(std::vector<Value> &&els)
      : elements(std::make_shared<std::vector<Value>>(std::move(els))),
        elementType(determineElementType(*elements)) {}

  // Constructor with explicit element type
  ArrayValue(const std::vector<Value> &els, ArrayElementType elemType)
      : elements(std::make_shared<std::vector<Value>>(els)),
        elementType(elemType) {}

  ArrayValue(std::vector<Value> &&els, ArrayElementType elemType)
      : elements(std::make_shared<std::vector<Value>>(std::move(els))),
        elementType(elemType) {}

  // Determine the element type based on the elements in the array
  static ArrayElementType
  determineElementType(const std::vector<Value> &elems) {
    if (elems.empty())
      return ArrayElementType::UNKNOWN;

    ArrayElementType firstType = getValueType(elems[0]);
    for (size_t i = 1; i < elems.size(); ++i) {
      if (getValueType(elems[i]) != firstType) {
        return ArrayElementType::MIXED;
      }
    }
    return firstType;
  }

  // Get the type of a value
  static ArrayElementType getValueType(const Value &val) {
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
  size_t size() const { return elements->size(); }

  // Check if the array is empty
  bool empty() const { return elements->empty(); }

  // Get element at index
  Value get(size_t index) const {
    if (index < elements->size())
      return (*elements)[index];
    else
      throw std::runtime_error("Array index out of bounds");
  }

  // Set element at index
  void set(size_t index, const Value &value) {
    if (index < elements->size())
      (*elements)[index] = value;
    else
      throw std::runtime_error("Array index out of bounds");
  }

  // Add element to the end
  void push_back(const Value &value) {
    elements->push_back(value);
    // Update type if needed
    if (elements->size() == 1) {
      elementType = getValueType(value);
    } else if (getValueType(value) != elementType) {
      elementType = ArrayElementType::MIXED;
    }
  }

  // Insert element at position
  void insert(size_t index, const Value &value) {
    if (index <= elements->size()) {
      elements->insert(elements->begin() + static_cast<std::ptrdiff_t>(index),
                       value);
      // Update type if needed
      if (elements->size() == 1) {
        elementType = getValueType(value);
      } else if (getValueType(value) != elementType) {
        elementType = ArrayElementType::MIXED;
      }
    } else {
      throw std::runtime_error("Array index out of bounds");
    }
  }

  // Remove element at index
  void removeAt(size_t index) {
    if (index < elements->size()) {
      elements->erase(elements->begin() + static_cast<std::ptrdiff_t>(index));
      // Recalculate type if empty
      if (elements->empty()) {
        elementType = ArrayElementType::UNKNOWN;
      } else if (elementType == ArrayElementType::MIXED) {
        // Maybe type is uniform now, but keeping mixed is safe/easier
        // Ideally we'd re-scan but that's O(N)
        elementType = determineElementType(*elements);
      }
    } else {
      throw std::runtime_error("Array index out of bounds");
    }
  }

  // Remove last element
  void pop_back() {
    if (!elements->empty()) {
      elements->pop_back();
      // Update type if needed
      if (elements->empty()) {
        elementType = ArrayElementType::UNKNOWN;
      } else {
        // Recalculate type
        elementType = determineElementType(*elements);
      }
    }
  }
};

// Helper function to create array value
inline ArrayValue makeArray(const std::vector<Value> &elements) {
  return ArrayValue(elements);
}

// Helper function to create typed array value
inline ArrayValue makeTypedArray(const std::vector<Value> &elements,
                                 ArrayElementType type) {
  return ArrayValue(elements, type);
}

// Helper function to convert ArrayValue to vector
inline std::vector<Value> getArray(const Value &value) {
  if (std::holds_alternative<ArrayValue>(value)) {
    return *(std::get<ArrayValue>(value).elements);
  }
  return std::vector<Value>();
}

// Equality operator for Value type
inline bool operator==(const Value &lhs, const Value &rhs) {
  // Compare types first
  if (lhs.index() != rhs.index()) {
    return false;
  }

  // Compare values based on type
  if (std::holds_alternative<int>(lhs)) {
    return std::get<int>(lhs) == std::get<int>(rhs);
  } else if (std::holds_alternative<double>(lhs)) {
    return std::get<double>(lhs) == std::get<double>(rhs);
  } else if (std::holds_alternative<bool>(lhs)) {
    return std::get<bool>(lhs) == std::get<bool>(rhs);
  } else if (std::holds_alternative<std::string>(lhs)) {
    return std::get<std::string>(lhs) == std::get<std::string>(rhs);
  } else if (std::holds_alternative<ArrayValue>(lhs)) {
    // Compare array elements
    const auto &lhsArr = *(std::get<ArrayValue>(lhs).elements);
    const auto &rhsArr = *(std::get<ArrayValue>(rhs).elements);
    if (lhsArr.size() != rhsArr.size()) {
      return false;
    }
    for (size_t i = 0; i < lhsArr.size(); ++i) {
      if (!(lhsArr[i] == rhsArr[i])) {
        return false;
      }
    }
    return true;
  }

  // Default case for unknown types
  return false;
}

// Environment for types during type checking
struct TypeEnvironment {
  std::unordered_map<std::string, std::shared_ptr<Type>> types;
  std::shared_ptr<TypeEnvironment> enclosing = nullptr;

  TypeEnvironment(std::shared_ptr<TypeEnvironment> parent = nullptr)
      : enclosing(parent) {}

  void define(const std::string &name, std::shared_ptr<Type> type) {
    types[name] = type;
  }

  std::shared_ptr<Type> get(const std::string &name) {
    auto it = types.find(name);
    if (it != types.end()) {
      return it->second;
    }
    if (enclosing) {
      return enclosing->get(name);
    }
    return nullptr;
  }
};

// Environment for variable bindings
struct Environment : public std::enable_shared_from_this<Environment> {
  std::unordered_map<std::string, Value> values;
  std::shared_ptr<Environment> enclosing = nullptr;
  bool is_block_scope = false;      // Flag to identify block vs function scope
  std::vector<Value> indexedValues; // Added for performance (O(1) access)

  Environment(std::shared_ptr<Environment> parent = nullptr,
              bool block_scope = false)
      : enclosing(parent), is_block_scope(block_scope) {}

  void define(const std::string &name, Value value);
  Value get(const std::string &name);
  void assign(const std::string &name, Value value);

  // Resolver optimization methods
  void defineAt(int index, Value value);
  Value getAt(int distance, int index);
  void assignAt(int distance, int index, Value value);
  std::shared_ptr<Environment> ancestor(int distance);
};

// Interpreter class
class Interpreter {
  friend struct EvalVisitor;
  friend struct ExecVisitor;
  friend struct TypeCheckVisitor;
  friend struct StmtTypeCheckVisitor;
  friend struct ResolverVisitor;

public:
  Interpreter();
  Value interpret(const Program &program);
  Value interpret(const Program &program, const std::vector<std::string> &args);
  Value interpret(const Program &program, const std::vector<std::string> &args,
                  const std::string &sourceName);

  void setSourceName(const std::string &name) { sourceName = name; }
  std::string getSourceName() const { return sourceName; }

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
  void visit(ClassDeclStmt &node);
  void visit(ConstructorDeclStmt &node);
  void visit(LambdaExpr &node);

private:
  std::shared_ptr<Environment> globals;
  std::shared_ptr<Environment> environment;
  std::shared_ptr<Environment>
      functionEnvironment; // Keep track of the function-level environment
  bool hasMainFunction;
  Statement::Ptr mainFunctionStmt; // Store reference to main function if found
  std::vector<std::string> commandLineArgs; // Store command-line arguments
  int evaluationDepth =
      0; // Track evaluation depth to prevent infinite recursion
  static constexpr int MAX_EVALUATION_DEPTH =
      50;                             // Maximum allowed evaluation depth
  std::vector<std::string> callStack; // Current call stack for tracing
  Value lastEvaluatedValue;
  std::string sourceName = "source.lin";
  Value evaluate(Expression &expr);
  Value evaluate(Expression::Ptr &exprPtr);
  void execute(Statement &stmt);
  Value executeBlock(const std::vector<Statement::Ptr> &statements,
                     std::shared_ptr<Environment> env);
  std::string valueToString(const Value &value);
  std::shared_ptr<Type> getTypeOfValue(const Value &value);
  std::string typeToString(const std::shared_ptr<Type> &type);

  // Type inference methods
  void performTypeInference(Program &program);
  void performTypeInferenceOnStatement(Statement &stmt,
                                       class TypeChecker &typeChecker);
  void performTypeInferenceOnExpression(Expression &expr,
                                        class TypeChecker &typeChecker);
  // Optimization methods
  void performOptimization(Program &program);

  // Helper method for function overloading
  std::shared_ptr<FunctionDef>
  findBestFunctionOverload(const std::string &name,
                           const std::vector<Value> &args);

  // Built-in function execution
  Value executeBuiltinFunction(
      const std::string &name,
      const std::vector<std::shared_ptr<Expression>> &arguments);

  // Function execution
  Value executeFunction(const std::string &name, Statement *body,
                        std::shared_ptr<Environment> funcEnv);

  // Static map to store function definitions for overload resolution
  static std::map<std::string, std::vector<std::shared_ptr<FunctionDef>>>
      functionDefinitions;

  // Map to store resolution (distance, index) for expressions (Resolver pass)
  // Key: Expression raw pointer (address), Value: pair(distance, index)
  std::map<const Expression *, std::pair<int, int>> locals;

  // API for Resolver
  void resolve(const Expression *expr, int depth, int index);
  // Helper to get resolved location
  std::optional<std::pair<int, int>>
  getResolvedLocation(const Expression *expr);
  void traceLookup(const std::string &name, std::optional<int> distance);
};

Value interpret(const Program &program);
Value interpret(const Program &program, const std::vector<std::string> &args);
Value interpret(const Program &program, const std::vector<std::string> &args,
                const std::string &sourceName);

// Utility functions
std::string getTypeOfValue(const Value &value);
std::string valueToString(const Value &value);
std::string typeToString(const std::shared_ptr<Type> &type);
bool valuesEqual(const Value &v1, const Value &v2);

// TypeChecker class for type checking
class TypeChecker {
public:
  std::shared_ptr<TypeEnvironment> typeEnvironment;
  std::shared_ptr<Environment> environment; // Still needed for some context

  TypeChecker(std::shared_ptr<TypeEnvironment> typeEnv,
              std::shared_ptr<Environment> env);

  // Helper method to get type of a value
  std::shared_ptr<Type> getTypeOfValue(const Value &value);

  // Helper method to convert type to string
  std::string typeToString(const std::shared_ptr<Type> &type);

  std::shared_ptr<Type> checkExpression(Expression &expr);
  void checkStatement(Statement &stmt);
};

} // namespace dotlin