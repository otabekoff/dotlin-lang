#include "dotlin/interpreter.h"
// #include <functional>
#include <iostream>
#include <map>
#include <memory>
// #include <sstream>
#include <stdexcept>
#include <vector>

namespace dotlin {

void Environment::define(const std::string &name, Value value) {
  values[name] = value;
}

Value Environment::get(const std::string &name) {
  auto it = values.find(name);
  if (it != values.end()) {
    return it->second;
  }

  if (enclosing) {
    return enclosing->get(name);
  }

  throw std::runtime_error("Undefined variable: " + name);
}

void Environment::assign(const std::string &name, Value value) {
  auto it = values.find(name);
  if (it != values.end()) {
    values[name] = value;
    return;
  }

  if (enclosing) {
    enclosing->assign(name, value);
    return;
  }

  throw std::runtime_error("Undefined variable: " + name);
}

Interpreter::Interpreter()
    : globals(std::make_shared<Environment>()), environment(globals),
      functionEnvironment(nullptr), hasMainFunction(false),
      mainFunctionStmt(nullptr), commandLineArgs({}) {}

Value Interpreter::interpret(const Program &program) {
  std::vector<std::string> empty_args;
  return interpret(program, empty_args);
}

Value Interpreter::interpret(const Program &program,
                             const std::vector<std::string> &args) {
  // Store command-line arguments
  commandLineArgs = args;

  // First, execute all statements to register functions and declare variables
  for (const auto &stmt : program.statements) {
    execute(*stmt);
  }

  if (hasMainFunction) {
    std::vector<Value> mainArgs;
    for (const auto &arg : commandLineArgs) {
      mainArgs.push_back(arg);
    }

    auto mainDef = findBestFunctionOverload("main", mainArgs);
    if (mainDef) {
      auto previousEnv = environment;
      auto previousFuncEnv = functionEnvironment;
      environment = std::make_shared<Environment>(globals, false);
      functionEnvironment = environment;

      for (size_t i = 0; i < mainDef->parameters.size(); ++i) {
        std::string paramName = mainDef->parameters[i].name;
        if (i < commandLineArgs.size()) {
          environment->define(paramName, commandLineArgs[i]);
        } else {
          Value defaultValue;
          if (mainDef->parameters[i].typeAnnotation.has_value() &&
              mainDef->parameters[i].typeAnnotation.value()) {
            auto type = mainDef->parameters[i].typeAnnotation.value();
            switch (type->kind) {
            case TypeKind::INT:
              defaultValue = 0;
              break;
            case TypeKind::DOUBLE:
              defaultValue = 0.0;
              break;
            case TypeKind::BOOL:
              defaultValue = false;
              break;
            case TypeKind::STRING:
              defaultValue = std::string("");
              break;
            default:
              defaultValue = std::string("");
              break;
            }
          } else {
            defaultValue = std::string("");
          }
          environment->define(paramName, defaultValue);
        }
      }

      if (mainDef->body) {
        try {
          if (auto *blockBody =
                  dynamic_cast<BlockStmt *>(mainDef->body.get())) {
            for (const auto &stmt : blockBody->statements) {
              if (stmt)
                execute(*stmt);
            }
          } else {
            execute(*mainDef->body);
          }
        } catch (const std::runtime_error &e) {
          std::string msg = e.what();
          if (msg.substr(0, 7) != "RETURN:") {
            throw;
          }
        }
      }

      environment = previousEnv;
      functionEnvironment = previousFuncEnv;
    }
  }

  // For now, return a dummy value
  return Value(std::string("Program executed successfully"));
}

Value interpret(const Program &program) {
  std::vector<std::string> empty_args;
  Interpreter interpreter;
  return interpreter.interpret(program, empty_args);
}

Value interpret(const Program &program, const std::vector<std::string> &args) {
  Interpreter interpreter;
  return interpreter.interpret(program, args);
}

// Visitor pattern implementations
void Interpreter::visit(LiteralExpr &node) {
  // The literal value is already stored in the node.value
  // Using the node to avoid unused parameter warning
  (void)node;
}

void Interpreter::visit(IdentifierExpr &node) {
  // Look up identifier in environment
  // This method is for statement execution, not expression evaluation
  // For expression evaluation, the visitor in evaluate() method is used
  (void)node;
}

void Interpreter::visit(BinaryExpr &node) {
  // Handle binary expressions based on operation type
  switch (node.op) {
  case TokenType::ASSIGN:
    // Handle assignment: left operand should be an identifier
    if (auto *identifier = dynamic_cast<IdentifierExpr *>(node.left.get())) {
      // Evaluate the right-hand side
      auto rightVal =
          node.right ? evaluate(*node.right) : Value(std::string("null"));

      std::cout << "DEBUG: Assignment statement - trying to assign "
                << identifier->name << " = ";
      std::cout << valueToString(rightVal) << std::endl;

      // Try to assign to the variable in the current environment first
      try {
        environment->assign(identifier->name, rightVal);
        std::cout
            << "DEBUG: Assignment statement successful in current environment"
            << std::endl;
      } catch (const std::exception &e) {
        std::cout
            << "DEBUG: Assignment statement failed in current environment: "
            << e.what() << std::endl;
        // If assignment fails in current environment, try the function
        // environment
        if (functionEnvironment && functionEnvironment != environment) {
          try {
            functionEnvironment->assign(identifier->name, rightVal);
            std::cout << "DEBUG: Assignment statement successful in function "
                         "environment"
                      << std::endl;
          } catch (const std::exception &e2) {
            std::cout << "DEBUG: Assignment statement failed in function "
                         "environment: "
                      << e2.what() << std::endl;
            // If assignment fails everywhere, define in the current environment
            environment->define(identifier->name, rightVal);
            std::cout << "DEBUG: Defined new variable in current environment"
                      << std::endl;
          }
        } else {
          // If no function environment or it's the same as current, define in
          // current
          environment->define(identifier->name, rightVal);
          std::cout << "DEBUG: Defined new variable in current environment"
                    << std::endl;
        }
      }
    }
    break;
  case TokenType::PLUS:
    // For arithmetic operations in statements, evaluate both sides
    evaluate(*node.left);
    evaluate(*node.right);
    break;
  case TokenType::MINUS:
    evaluate(*node.left);
    evaluate(*node.right);
    break;
  case TokenType::MULTIPLY:
    evaluate(*node.left);
    evaluate(*node.right);
    break;
  case TokenType::DIVIDE:
    evaluate(*node.left);
    evaluate(*node.right);
    break;
  case TokenType::EQUAL:
    // For comparison in statements, evaluate both sides
    evaluate(*node.left);
    evaluate(*node.right);
    break;
  default:
    // For other operations, just evaluate both sides
    if (node.left)
      evaluate(*node.left);
    if (node.right)
      evaluate(*node.right);
    break;
  }
}

void Interpreter::visit(UnaryExpr &node) {
  // Evaluate unary expressions
  auto operandValue =
      node.operand ? evaluate(*node.operand) : Value(std::string("null"));

  // Perform the operation based on node.op
  switch (node.op) {
  case TokenType::NOT:
    // Handle logical NOT
    break;
  case TokenType::MINUS:
    // Handle unary minus
    break;
  default:
    break;
  }
}

void Interpreter::visit(CallExpr &node) {
  // Handle function calls
  // For statement execution, we primarily care about side effects like printing
  std::cout << "DEBUG: In CallExpr evaluator (statement)" << std::endl;
  std::cout << "DEBUG: Number of args: " << node.arguments.size() << std::endl;

  // Check if this is a built-in function call
  if (auto *identifier = dynamic_cast<IdentifierExpr *>(node.callee.get())) {
    std::string funcName = identifier->name;
    if (funcName == "println") {
      if (!node.arguments.empty() && node.arguments[0]) {
        auto argValue = evaluate(*node.arguments[0]);
        std::cout << valueToString(argValue);
      }
      std::cout << std::endl; // Add newline for println
      return;
    } else if (funcName == "print") {
      if (!node.arguments.empty() && node.arguments[0]) {
        auto argValue = evaluate(*node.arguments[0]);
        std::cout << valueToString(argValue);
      }
      return;
    } else if (funcName == "readln") {
      std::string input;
      std::getline(std::cin, input);
      // Store the result somehow - for now just return
      return;
    }
  }

  // For other function calls, evaluate arguments but don't do anything
  for (const auto &arg : node.arguments) {
    if (arg) {
      evaluate(*arg);
    }
  }
}

void Interpreter::visit(MemberAccessExpr &node) {
  // Handle member access for statement execution
  // We need to evaluate the expression to trigger any side effects
  evaluate(node);
}

void Interpreter::visit(VariableDeclStmt &node) {
  Value value;
  if (node.initializer.has_value()) {
    // Evaluate the initializer expression
    value = evaluate(*(node.initializer.value()));

    // If type annotation exists, validate the type
    if (node.typeAnnotation.has_value() && node.typeAnnotation.value()) {
      auto expectedType = node.typeAnnotation.value();
      auto actualType = getTypeOfValue(value);

      if (!expectedType->isCompatibleWith(*actualType)) {
        std::cout << "Type error: variable " << node.name
                  << " declared with type " << typeToString(expectedType)
                  << " but initialized with incompatible type" << std::endl;
      }
    } else {
      // Type inference: infer the type from the initializer value
      auto inferredType = getTypeOfValue(value);
      // In a real implementation, we might want to store the inferred type in
      // the environment For now, just use the value as is
    }
  } else {
    // Default value for uninitialized variables
    if (node.typeAnnotation.has_value() && node.typeAnnotation.value()) {
      // Initialize with default value based on type annotation
      auto type = node.typeAnnotation.value();
      switch (type->kind) {
      case TypeKind::INT:
        value = 0;
        break;
      case TypeKind::DOUBLE:
        value = 0.0;
        break;
      case TypeKind::BOOL:
        value = false;
        break;
      case TypeKind::STRING:
        value = std::string("");
        break;
      default:
        value = 0; // Default to integer 0
        break;
      }
    } else {
      // Type inference: no type annotation and no initializer, use
      // unknown/default
      value = 0; // Default to integer 0 for now
    }
  }

  environment->define(node.name, value);
}

// Static map to store function definitions - now maps function name to vector
// of overloads
static std::map<std::string, std::vector<std::shared_ptr<FunctionDef>>>
    functionDefinitions;

void Interpreter::visit(FunctionDeclStmt &node) {
  // Store function in environment for later use as a special marker
  std::string funcMarker = "function:" + node.name;
  environment->define(node.name, Value(funcMarker));

  // Create a function definition object
  std::vector<FunctionParameter> paramsCopy;
  for (const auto &param : node.parameters) {
    paramsCopy.push_back(param);
  }

  auto funcDef = std::make_shared<FunctionDef>(node.name, std::move(paramsCopy),
                                               std::move(node.body));
  // Store in a map so we can retrieve it later - now appending to vector for
  // overloading support
  functionDefinitions[node.name].push_back(funcDef);

  if (node.name == "main") {
    hasMainFunction = true;
  }
}

void Interpreter::visit(BlockStmt &node) {
  // Execute block with new environment that marks this as a block scope
  executeBlock(node.statements, environment);
}

void Interpreter::visit(ReturnStmt &node) {
  // Handle return statements by throwing an exception with the return value
  if (node.value) {
    Value retVal = evaluate(*node.value);
    std::string returnStr = "RETURN:" + valueToString(retVal);
    throw std::runtime_error(returnStr);
  } else {
    // Default return value
    throw std::runtime_error("RETURN:undefined");
  }
}

void Interpreter::visit(IfStmt &node) {
  // Handle if statements
  auto conditionValue =
      node.condition ? evaluate(*node.condition) : Value(false);

  // In a real implementation, would execute appropriate branch based on
  // condition
  if (std::holds_alternative<bool>(conditionValue) &&
      std::get<bool>(conditionValue)) {
    if (node.thenBranch)
      execute(*node.thenBranch);
  } else if (node.elseBranch.has_value() && node.elseBranch.value()) {
    execute(*node.elseBranch.value());
  }
}

void Interpreter::visit(WhileStmt &node) {
  // Execute the while loop
  while (true) {
    // Evaluate the condition
    auto conditionValue =
        node.condition ? evaluate(*node.condition) : Value(false);

    // Check if the condition is true
    bool shouldContinue = false;
    if (std::holds_alternative<bool>(conditionValue)) {
      shouldContinue = std::get<bool>(conditionValue);
    } else if (std::holds_alternative<int>(conditionValue)) {
      // Treat non-zero integers as true
      shouldContinue = std::get<int>(conditionValue) != 0;
    } else if (std::holds_alternative<std::string>(conditionValue)) {
      // Treat non-empty strings as true
      shouldContinue = !std::get<std::string>(conditionValue).empty();
    }

    if (!shouldContinue) {
      break; // Exit the loop if condition is false
    }

    // Execute the loop body
    if (node.body) {
      // If the body is a block, execute its statements directly in the current
      // environment so that variable changes persist across iterations
      if (auto *blockBody = dynamic_cast<BlockStmt *>(node.body.get())) {
        for (const auto &stmt : blockBody->statements) {
          if (stmt)
            execute(*stmt);
        }
      } else {
        // For non-block bodies, execute normally
        execute(*node.body);
      }
    }
  }
}

void Interpreter::visit(ForStmt &node) {
  // Evaluate the iterable expression
  Value iterableValue;
  if (node.iterable) {
    iterableValue = evaluate(*node.iterable);
  } else {
    return; // Nothing to iterate over
  }

  // Handle iteration over an array
  if (std::holds_alternative<ArrayValue>(iterableValue)) {
    const ArrayValue &arr = std::get<ArrayValue>(iterableValue);

    // Create a new environment for the loop scope
    auto loopEnv = std::make_shared<Environment>(environment);

    // Iterate over each element in the array
    for (const auto &element : arr.elements) {
      // Define the loop variable in the loop environment
      loopEnv->define(node.variable, element);

      // Execute the loop body with the current loop environment
      if (node.body) {
        auto prevEnv = environment;
        environment = loopEnv;
        execute(*node.body);
        environment = prevEnv;
      }
    }
  }
  // Handle iteration over integer ranges (e.g., for (i in 0..10))
  // For now, we'll implement basic range iteration if the iterable is a range
  // Note: This would require implementing range expressions separately
  else {
    // For other types, we might want to implement different iteration
    // strategies For now, just return
    return;
  }
}

void Interpreter::visit(WhenStmt &node) {
  // Evaluate the subject expression
  Value subjectValue;
  if (node.subject) {
    subjectValue = evaluate(*node.subject);
  } else {
    return; // Nothing to match
  }

  // Try each branch pattern to find a match
  bool matched = false;
  for (const auto &branch : node.branches) {
    // Evaluate the pattern to compare with the subject
    if (branch.first) // pattern
    {
      Value patternValue = evaluate(*branch.first);

      // Simple equality comparison for now
      if (subjectValue == patternValue) {
        // Execute the corresponding statement for this branch
        if (branch.second) // statement
        {
          execute(*branch.second);
        }
        matched = true;
        break; // Exit after first match
      }
    }
  }

  // If no branch matched and there's an else branch, execute it
  if (!matched && node.elseBranch.has_value() && node.elseBranch.value()) {
    execute(*node.elseBranch.value());
  }
}

void Interpreter::visit(TryStmt &node) {
  // Execute the try block with exception handling
  bool exceptionOccurred = false;
  std::string exceptionMessage = "";

  try {
    if (node.tryBlock) {
      execute(*node.tryBlock);
    }
  } catch (const std::exception &e) {
    exceptionOccurred = true;
    exceptionMessage = e.what();
  } catch (...) {
    exceptionOccurred = true;
    exceptionMessage = "Unknown exception occurred";
  }

  // If an exception occurred, execute the catch block
  if (exceptionOccurred && node.catchBlock) {
    // Create new environment for catch block to add exception variable
    auto catchEnv = std::make_shared<Environment>(environment);
    catchEnv->define(node.exceptionVar, exceptionMessage);

    // Save current environment
    auto prevEnv = environment;
    environment = catchEnv;

    // Execute catch block
    execute(*node.catchBlock);

    // Restore environment
    environment = prevEnv;
  }

  // Execute finally block if present (always executes)
  if (node.finallyBlock.has_value() && node.finallyBlock.value()) {
    execute(*node.finallyBlock.value());
  }
}

void Interpreter::visit(ExpressionStmt &node) {
  // Execute the expression for its side effects
  std::cout << "DEBUG: Executing ExpressionStmt" << std::endl;
  if (auto *callExpr = dynamic_cast<CallExpr *>(node.expression.get())) {
    std::cout << "DEBUG: Expression is a CallExpr" << std::endl;
    (void)callExpr; // Suppress unused variable warning
  }
  evaluate(*node.expression);
}

void Interpreter::visit(ClassDeclStmt &node) {
  // Create a class definition
  auto classDef = std::make_shared<ClassDefinition>(node.name);

  // Process class members to identify fields, constructors, and methods
  for (const auto &member : node.members) {
    if (auto *fieldDecl = dynamic_cast<VariableDeclStmt *>(member.get())) {
      // Add field to class definition
      if (fieldDecl->typeAnnotation.has_value() &&
          fieldDecl->typeAnnotation.value()) {
        classDef->fields.push_back(
            {fieldDecl->name, fieldDecl->typeAnnotation.value()});
      } else {
        // If no type annotation, use unknown type
        classDef->fields.push_back(
            {fieldDecl->name, std::make_shared<Type>(TypeKind::UNKNOWN)});
      }
    } else if (auto *constructorDecl =
                   dynamic_cast<ConstructorDeclStmt *>(member.get())) {
      // Create function definition for constructor
      std::vector<FunctionParameter> params;
      for (const auto &param : constructorDecl->parameters) {
        params.push_back(param);
      }

      // Create a function definition for constructor
      auto constructorDef = std::make_shared<FunctionDef>(
          "constructor", std::move(params),
          Statement::Ptr(constructorDecl->body.get()));
      classDef->constructors.push_back(constructorDef);
    } else if (auto *methodDecl =
                   dynamic_cast<FunctionDeclStmt *>(member.get())) {
      // Create function definition for method
      std::vector<FunctionParameter> params;
      for (const auto &param : methodDecl->parameters) {
        params.push_back(param);
      }

      // Create a function definition for method
      auto methodDef =
          std::make_shared<FunctionDef>(methodDecl->name, std::move(params),
                                        Statement::Ptr(methodDecl->body.get()));
      classDef->methods.push_back(methodDef);
    }
  }

  // Store the class definition in the environment
  environment->define(node.name, Value(classDef));
}

void Interpreter::visit(ConstructorDeclStmt &node) {
  // Constructor is handled as part of class declaration, no separate processing
  // needed here The constructor is already processed when visiting the
  // ClassDeclStmt
  (void)node; // Suppress unused parameter warning
}

void Interpreter::visit(LambdaExpr &node) {
  // For now, we'll just acknowledge the lambda expression
  // The actual lambda value creation and handling happens in the expression
  // evaluation visitor This method is for statement execution context, not
  // expression evaluation
  (void)node; // Suppress unused parameter warning
}

Value Interpreter::evaluate(Expression &expr) {
  // Check for potential null expression (this shouldn't happen in normal cases,
  // but as a safeguard) Increment evaluation depth and check for recursion
  // limit
  evaluationDepth++;
  if (evaluationDepth > MAX_EVALUATION_DEPTH) {
    std::cout << "ERROR: Maximum evaluation depth exceeded, possible infinite "
                 "recursion"
              << std::endl;
    evaluationDepth--; // Decrement before returning
    return std::string("recursion_limit_exceeded");
  }

  // Use the visitor pattern to dispatch to the appropriate visit method
  struct EvalVisitor : public AstVisitor {
    Value result;
    Interpreter *interpreter;

    EvalVisitor(Interpreter *interp) : interpreter(interp) {}

    void visit(LiteralExpr &node) override {
      // Return the literal value directly
      // Convert the old variant type to the new Value type
      if (std::holds_alternative<int>(node.value)) {
        result = Value(std::get<int>(node.value));
      } else if (std::holds_alternative<double>(node.value)) {
        result = Value(std::get<double>(node.value));
      } else if (std::holds_alternative<bool>(node.value)) {
        result = Value(std::get<bool>(node.value));
      } else if (std::holds_alternative<std::string>(node.value)) {
        result = Value(std::get<std::string>(node.value));
      } else {
        result = std::string("undefined");
      }
    }

    void visit(IdentifierExpr &node) override {
      // Special handling for "args" variable
      if (node.name == "args") {
        // Create an array-like representation of command-line arguments
        std::vector<Value> argsVector;
        for (const auto &arg : interpreter->commandLineArgs) {
          argsVector.push_back(arg);
        }
        result = Value(ArrayValue(std::move(argsVector)));
      }
      // Special handling for "this" keyword
      else if (node.name == "this") {
        std::cout << "DEBUG: Looking up 'this' in environment" << std::endl;
        try {
          result = interpreter->environment->get("this");
          std::cout << "DEBUG: Found 'this' in environment" << std::endl;
        } catch (const std::exception &e) {
          std::cout << "DEBUG: 'this' not found in environment: " << e.what()
                    << std::endl;
          result = Value(std::string("undefined"));
        }
      }
      // Look up the value in the environment
      else {
        try {
          result = interpreter->environment->get(
              node.name); // This should already return Value type
        } catch (const std::exception &e) {
          // Return a default value if variable not found
          result = Value(std::string("undefined"));
        }
      }
    }

    void visit(LambdaExpr &node) override {
      // Create a LambdaValue representing the lambda expression
      // Capture the current environment as the closure
      // Since we can't copy unique_ptr from the original AST, we'll create a
      // new LambdaValue with copied parameters and a nullptr for body (we'll
      // handle execution differently)
      std::vector<FunctionParameter> paramsCopy;
      for (const auto &param : node.parameters) {
        paramsCopy.push_back(param); // Copy each parameter individually
      }

      // We can't copy the body (unique_ptr), so we'll need to handle lambda
      // execution by referencing the original AST node when the lambda is
      // called For now, pass a nullptr as body, and store a reference to the
      // original node
      auto lambdaValue = std::make_shared<LambdaValue>(
          std::move(paramsCopy), nullptr, interpreter->environment, &node);
      result = lambdaValue;
    }

    void visit(BinaryExpr &node) override {
      // Handle assignment specially - don't evaluate left operand as a value
      if (node.op == TokenType::ASSIGN) {
        if (auto *identifier =
                dynamic_cast<IdentifierExpr *>(node.left.get())) {
          // Evaluate the right-hand side
          auto rightVal = node.right ? interpreter->evaluate(*node.right)
                                     : Value(std::string("null"));

          // Try to assign to the variable in the current environment first
          try {
            interpreter->environment->assign(identifier->name, rightVal);
            result = rightVal; // Assignment returns the assigned value
          } catch (const std::exception &e) {
            // If assignment fails in current environment, try the function
            // environment
            if (interpreter->functionEnvironment &&
                interpreter->functionEnvironment != interpreter->environment) {
              try {
                interpreter->functionEnvironment->assign(identifier->name,
                                                         rightVal);
                result = rightVal; // Assignment returns the assigned value
              } catch (const std::exception &e2) {
                // If assignment fails everywhere, define in the current
                // environment
                interpreter->environment->define(identifier->name, rightVal);
                result = rightVal; // Assignment returns the assigned value
              }
            } else {
              // If no function environment or it's the same as current, define
              // in current
              interpreter->environment->define(identifier->name, rightVal);
              result = rightVal; // Assignment returns the assigned value
            }
          }
          return; // Early return after handling assignment
        } else if (auto *memberAccess =
                       dynamic_cast<MemberAccessExpr *>(node.left.get())) {
          // Handle assignment to member access (e.g., this.name = value)
          auto rightVal = node.right ? interpreter->evaluate(*node.right)
                                     : Value(std::string("null"));

          // Evaluate the object to get the instance
          if (memberAccess->object) {
            auto objValue = interpreter->evaluate(*memberAccess->object);

            // Check if the object is a ClassInstance
            if (std::holds_alternative<std::shared_ptr<ClassInstance>>(
                    objValue)) {
              auto instance =
                  std::get<std::shared_ptr<ClassInstance>>(objValue);

              // Set the field value
              instance->fields[memberAccess->property] = rightVal;
              result = rightVal; // Assignment returns the assigned value
              return;
            }
          }

          // If we get here, the assignment failed
          result = 0;
          return;
        } else {
          // Assignment to non-identifier/non-member-access is invalid
          result = 0;
          return;
        }
      }

      // For non-assignment operations, evaluate both operands and perform
      // operation
      auto leftVal = node.left ? interpreter->evaluate(*node.left)
                               : Value(std::string("null"));
      auto rightVal = node.right ? interpreter->evaluate(*node.right)
                                 : Value(std::string("null"));

      // Handle different type combinations for operations
      switch (node.op) {
      case TokenType::PLUS:
        // String concatenation or numeric addition
        if (std::holds_alternative<std::string>(leftVal) ||
            std::holds_alternative<std::string>(rightVal)) {
          std::string leftStr, rightStr;
          if (std::holds_alternative<int>(leftVal)) {
            leftStr = std::to_string(std::get<int>(leftVal));
          } else if (std::holds_alternative<double>(leftVal)) {
            leftStr = std::to_string(std::get<double>(leftVal));
          } else if (std::holds_alternative<bool>(leftVal)) {
            leftStr = std::get<bool>(leftVal) ? "true" : "false";
          } else {
            leftStr = std::get<std::string>(leftVal);
          }
          if (std::holds_alternative<int>(rightVal)) {
            rightStr = std::to_string(std::get<int>(rightVal));
          } else if (std::holds_alternative<double>(rightVal)) {
            rightStr = std::to_string(std::get<double>(rightVal));
          } else if (std::holds_alternative<bool>(rightVal)) {
            rightStr = std::get<bool>(rightVal) ? "true" : "false";
          } else {
            rightStr = std::get<std::string>(rightVal);
          }
          result = Value(leftStr + rightStr);
        } else if (std::holds_alternative<int>(leftVal) &&
                   std::holds_alternative<int>(rightVal)) {
          result = Value(std::get<int>(leftVal) + std::get<int>(rightVal));
        } else if (std::holds_alternative<double>(leftVal) ||
                   std::holds_alternative<double>(rightVal)) {
          double left = std::holds_alternative<int>(leftVal)
                            ? static_cast<double>(std::get<int>(leftVal))
                            : std::get<double>(leftVal);
          double right = std::holds_alternative<int>(rightVal)
                             ? static_cast<double>(std::get<int>(rightVal))
                             : std::get<double>(rightVal);
          result = Value(left + right);
        } else {
          result = 0; // Default for unsupported operations
        }
        break;
      case TokenType::MINUS:
        if (std::holds_alternative<int>(leftVal) &&
            std::holds_alternative<int>(rightVal)) {
          result = Value(std::get<int>(leftVal) - std::get<int>(rightVal));
        } else if (std::holds_alternative<double>(leftVal) ||
                   std::holds_alternative<double>(rightVal)) {
          double left = std::holds_alternative<int>(leftVal)
                            ? static_cast<double>(std::get<int>(leftVal))
                            : std::get<double>(leftVal);
          double right = std::holds_alternative<int>(rightVal)
                             ? static_cast<double>(std::get<int>(rightVal))
                             : std::get<double>(rightVal);
          result = Value(left - right);
        } else {
          result = 0; // Default for unsupported operations
        }
        break;
      case TokenType::MULTIPLY:
        if (std::holds_alternative<int>(leftVal) &&
            std::holds_alternative<int>(rightVal)) {
          result = Value(std::get<int>(leftVal) * std::get<int>(rightVal));
        } else if (std::holds_alternative<double>(leftVal) ||
                   std::holds_alternative<double>(rightVal)) {
          double left = std::holds_alternative<int>(leftVal)
                            ? static_cast<double>(std::get<int>(leftVal))
                            : std::get<double>(leftVal);
          double right = std::holds_alternative<int>(rightVal)
                             ? static_cast<double>(std::get<int>(rightVal))
                             : std::get<double>(rightVal);
          result = Value(left * right);
        } else {
          result = 0; // Default for unsupported operations
        }
        break;
      case TokenType::DIVIDE:
        if (std::holds_alternative<int>(leftVal) &&
            std::holds_alternative<int>(rightVal)) {
          result = Value(std::get<int>(rightVal) != 0
                             ? std::get<int>(leftVal) / std::get<int>(rightVal)
                             : 0);
        } else if (std::holds_alternative<double>(leftVal) ||
                   std::holds_alternative<double>(rightVal)) {
          double left = std::holds_alternative<int>(leftVal)
                            ? static_cast<double>(std::get<int>(leftVal))
                            : std::get<double>(leftVal);
          double right = std::holds_alternative<int>(rightVal)
                             ? static_cast<double>(std::get<int>(rightVal))
                             : std::get<double>(rightVal);
          result = Value(right != 0 ? left / right : 0.0);
        } else {
          result = 0; // Default for unsupported operations
        }
        break;
      case TokenType::MODULO:
        if (std::holds_alternative<int>(leftVal) &&
            std::holds_alternative<int>(rightVal)) {
          result = Value(std::get<int>(rightVal) != 0
                             ? std::get<int>(leftVal) % std::get<int>(rightVal)
                             : 0);
        } else {
          result = 0; // Default for unsupported operations
        }
        break;
      case TokenType::EQUAL:
        // Equality comparison for different types
        if (std::holds_alternative<int>(leftVal) &&
            std::holds_alternative<int>(rightVal)) {
          result = Value(std::get<int>(leftVal) == std::get<int>(rightVal));
        } else if (std::holds_alternative<double>(leftVal) ||
                   std::holds_alternative<double>(rightVal)) {
          double left = std::holds_alternative<int>(leftVal)
                            ? static_cast<double>(std::get<int>(leftVal))
                            : std::get<double>(leftVal);
          double right = std::holds_alternative<int>(rightVal)
                             ? static_cast<double>(std::get<int>(rightVal))
                             : std::get<double>(rightVal);
          result = Value(left == right);
        } else if (std::holds_alternative<std::string>(leftVal) &&
                   std::holds_alternative<std::string>(rightVal)) {
          result = Value(std::get<std::string>(leftVal) ==
                         std::get<std::string>(rightVal));
        } else if (std::holds_alternative<bool>(leftVal) &&
                   std::holds_alternative<bool>(rightVal)) {
          result = std::get<bool>(leftVal) == std::get<bool>(rightVal);
        } else {
          result = false; // Default for unsupported comparisons
        }
        break;
      case TokenType::NOT_EQUAL:
        if (std::holds_alternative<int>(leftVal) &&
            std::holds_alternative<int>(rightVal)) {
          result = std::get<int>(leftVal) != std::get<int>(rightVal);
        } else if (std::holds_alternative<double>(leftVal) ||
                   std::holds_alternative<double>(rightVal)) {
          double left = std::holds_alternative<int>(leftVal)
                            ? static_cast<double>(std::get<int>(leftVal))
                            : std::get<double>(leftVal);
          double right = std::holds_alternative<int>(rightVal)
                             ? static_cast<double>(std::get<int>(rightVal))
                             : std::get<double>(rightVal);
          result = left != right;
        } else if (std::holds_alternative<std::string>(leftVal) &&
                   std::holds_alternative<std::string>(rightVal)) {
          result =
              std::get<std::string>(leftVal) != std::get<std::string>(rightVal);
        } else if (std::holds_alternative<bool>(leftVal) &&
                   std::holds_alternative<bool>(rightVal)) {
          result = std::get<bool>(leftVal) != std::get<bool>(rightVal);
        } else {
          result = true; // Default for unsupported comparisons
        }
        break;
      case TokenType::LESS:
        if (std::holds_alternative<int>(leftVal) &&
            std::holds_alternative<int>(rightVal)) {
          result = std::get<int>(leftVal) < std::get<int>(rightVal);
        } else if (std::holds_alternative<double>(leftVal) ||
                   std::holds_alternative<double>(rightVal)) {
          double left = std::holds_alternative<int>(leftVal)
                            ? static_cast<double>(std::get<int>(leftVal))
                            : std::get<double>(leftVal);
          double right = std::holds_alternative<int>(rightVal)
                             ? static_cast<double>(std::get<int>(rightVal))
                             : std::get<double>(rightVal);
          result = left < right;
        } else {
          result = false; // Default for unsupported comparisons
        }
        break;
      case TokenType::LESS_EQUAL:
        if (std::holds_alternative<int>(leftVal) &&
            std::holds_alternative<int>(rightVal)) {
          result = std::get<int>(leftVal) <= std::get<int>(rightVal);
        } else if (std::holds_alternative<double>(leftVal) ||
                   std::holds_alternative<double>(rightVal)) {
          double left = std::holds_alternative<int>(leftVal)
                            ? static_cast<double>(std::get<int>(leftVal))
                            : std::get<double>(leftVal);
          double right = std::holds_alternative<int>(rightVal)
                             ? static_cast<double>(std::get<int>(rightVal))
                             : std::get<double>(rightVal);
          result = left <= right;
        } else {
          result = false; // Default for unsupported comparisons
        }
        break;
      case TokenType::GREATER:
        if (std::holds_alternative<int>(leftVal) &&
            std::holds_alternative<int>(rightVal)) {
          result = std::get<int>(leftVal) > std::get<int>(rightVal);
        } else if (std::holds_alternative<double>(leftVal) ||
                   std::holds_alternative<double>(rightVal)) {
          double left = std::holds_alternative<int>(leftVal)
                            ? static_cast<double>(std::get<int>(leftVal))
                            : std::get<double>(leftVal);
          double right = std::holds_alternative<int>(rightVal)
                             ? static_cast<double>(std::get<int>(rightVal))
                             : std::get<double>(rightVal);
          result = left > right;
        } else {
          result = false; // Default for unsupported comparisons
        }
        break;
      case TokenType::GREATER_EQUAL:
        if (std::holds_alternative<int>(leftVal) &&
            std::holds_alternative<int>(rightVal)) {
          result = std::get<int>(leftVal) >= std::get<int>(rightVal);
        } else if (std::holds_alternative<double>(leftVal) ||
                   std::holds_alternative<double>(rightVal)) {
          double left = std::holds_alternative<int>(leftVal)
                            ? static_cast<double>(std::get<int>(leftVal))
                            : std::get<double>(leftVal);
          double right = std::holds_alternative<int>(rightVal)
                             ? static_cast<double>(std::get<int>(rightVal))
                             : std::get<double>(rightVal);
          result = left >= right;
        } else {
          result = false; // Default for unsupported comparisons
        }
        break;
      case TokenType::AND:
        if (std::holds_alternative<bool>(leftVal) &&
            std::holds_alternative<bool>(rightVal)) {
          result = std::get<bool>(leftVal) && std::get<bool>(rightVal);
        } else {
          result = false; // Default for unsupported operations
        }
        break;
      case TokenType::OR:
        if (std::holds_alternative<bool>(leftVal) &&
            std::holds_alternative<bool>(rightVal)) {
          result = std::get<bool>(leftVal) || std::get<bool>(rightVal);
        } else {
          result = false; // Default for unsupported operations
        }
        break;

      default:
        result = 0; // Default for unsupported operations
        break;
      }
    }

    void visit(UnaryExpr &node) override {
      auto operandVal = node.operand ? interpreter->evaluate(*node.operand)
                                     : Value(std::string("null"));

      switch (node.op) {
      case TokenType::MINUS:
        if (std::holds_alternative<int>(operandVal)) {
          result = -std::get<int>(operandVal);
        } else if (std::holds_alternative<double>(operandVal)) {
          result = -std::get<double>(operandVal);
        } else {
          result = 0; // Default for unsupported operations
        }
        break;
      case TokenType::NOT:
        if (std::holds_alternative<bool>(operandVal)) {
          result = !std::get<bool>(operandVal);
        } else if (std::holds_alternative<int>(operandVal)) {
          // For integers, treat 0 as false and non-zero as true
          result = !std::get<int>(operandVal);
        } else {
          result = false; // Default for unsupported operations
        }
        break;
      case TokenType::INCREMENT:
        if (std::holds_alternative<int>(operandVal)) {
          result = std::get<int>(operandVal) + 1;
        } else if (std::holds_alternative<double>(operandVal)) {
          result = std::get<double>(operandVal) + 1.0;
        } else {
          result = 1; // Default for unsupported operations
        }
        break;
      case TokenType::DECREMENT:
        if (std::holds_alternative<int>(operandVal)) {
          result = std::get<int>(operandVal) - 1;
        } else if (std::holds_alternative<double>(operandVal)) {
          result = std::get<double>(operandVal) - 1.0;
        } else {
          result = -1; // Default for unsupported operations
        }
        break;
      default:
        result = operandVal; // Return as-is for other types
        break;
      }
    }

    void visit(CallExpr &node) override {
      // Handle function calls

      // Evaluate arguments
      std::vector<Value> args;
      for (const auto &arg : node.arguments) {
        if (arg) {
          args.push_back(interpreter->evaluate(*arg));
        }
      }

      // Check if this is a method call on a class instance
      if (auto *memberAccess =
              dynamic_cast<MemberAccessExpr *>(node.callee.get())) {
        std::cout << "DEBUG: CallExpr callee is a MemberAccessExpr"
                  << std::endl;
        // Handle method calls like obj.method()
        auto objValue = interpreter->evaluate(*memberAccess->object);
        std::string methodName = memberAccess->property;

        // Check if it's the special case of args.size() or
        // args.contentToString()
        std::string objName = "";
        bool isArgsObject = false;
        auto *ident =
            dynamic_cast<IdentifierExpr *>(memberAccess->object.get());
        if (ident) {
          objName = ident->name;
          isArgsObject = (objName == "args");
        }

        if (isArgsObject) {
          // methodName is already declared above, reuse it

          if (objName == "args" && methodName == "size" && args.empty()) {
            // Handle args.size() method call
            result = static_cast<int>(interpreter->commandLineArgs.size());
            return;
          } else if (objName == "args" && methodName == "contentToString" &&
                     args.empty()) {
            // Handle args.contentToString() method call
            std::string content = "[";
            for (size_t i = 0; i < interpreter->commandLineArgs.size(); ++i) {
              content += interpreter->commandLineArgs[i];
              if (i < interpreter->commandLineArgs.size() - 1) {
                content += ", ";
              }
            }
            content += "]";
            result = content;
            return;
          } else {
            // Unrecognized method on args, return default
            result = std::string("");
            return;
          }
        } else {
          // Check if the object of the member access is a call expression or
          // member access (chained method call) This handles cases like:
          // expr.method().otherMethod() or expr.method1().method2()
          if ((dynamic_cast<CallExpr *>(memberAccess->object.get()) ||
               dynamic_cast<MemberAccessExpr *>(memberAccess->object.get()))) {
            // This is a chained method call like: expr.method().otherMethod()
            // First evaluate the object (which could be a call or another
            // member access) to get the base value
            Value baseValue = memberAccess->object
                                  ? interpreter->evaluate(*memberAccess->object)
                                  : Value(std::string("null"));

            // Now handle method calls on the result
            // methodName is already declared above, reuse it

            if (methodName == "toString" && args.empty()) {
              result = interpreter->valueToString(baseValue);
              return;
            } else if (methodName == "substring" && args.size() == 1 &&
                       std::holds_alternative<int>(args[0])) {
              // Handle substring(start) method call
              if (std::holds_alternative<std::string>(baseValue)) {
                std::string str = std::get<std::string>(baseValue);
                int start = std::get<int>(args[0]);
                if (start >= 0 && static_cast<size_t>(start) <= str.length()) {
                  result = str.substr(static_cast<size_t>(start));
                } else {
                  result =
                      std::string(""); // Return empty string for invalid index
                }
              } else {
                result = std::string(
                    ""); // Return empty string for non-string objects
              }
              return;
            } else if (methodName == "substring" && args.size() == 2 &&
                       std::holds_alternative<int>(args[0]) &&
                       std::holds_alternative<int>(args[1])) {
              // Handle substring(start, end) method call
              if (std::holds_alternative<std::string>(baseValue)) {
                std::string str = std::get<std::string>(baseValue);
                int start = std::get<int>(args[0]);
                int end = std::get<int>(args[1]);
                if (start >= 0 && static_cast<size_t>(end) <= str.length() &&
                    start <= end) {
                  result = str.substr(static_cast<size_t>(start),
                                      static_cast<size_t>(end - start));
                } else {
                  result =
                      std::string(""); // Return empty string for invalid range
                }
              } else {
                result = std::string(
                    ""); // Return empty string for non-string objects
              }
              return;
            } else if (methodName == "indexOf" && args.size() == 1 &&
                       std::holds_alternative<std::string>(args[0])) {
              // Handle indexOf(substring) method call
              if (std::holds_alternative<std::string>(baseValue)) {
                std::string str = std::get<std::string>(baseValue);
                std::string substr = std::get<std::string>(args[0]);
                size_t pos = str.find(substr);
                result = static_cast<int>(
                    pos != std::string::npos ? static_cast<int>(pos) : -1);
              } else {
                result = -1; // Return -1 for non-string objects
              }
              return;
            } else if (methodName == "startsWith" && args.size() == 1 &&
                       std::holds_alternative<std::string>(args[0])) {
              // Handle startsWith(prefix) method call
              if (std::holds_alternative<std::string>(baseValue)) {
                std::string str = std::get<std::string>(baseValue);
                std::string prefix = std::get<std::string>(args[0]);
                result =
                    static_cast<bool>(str.substr(0, prefix.length()) == prefix);
              } else {
                result = false; // Return false for non-string objects
              }
              return;
            } else if (methodName == "endsWith" && args.size() == 1 &&
                       std::holds_alternative<std::string>(args[0])) {
              // Handle endsWith(suffix) method call
              if (std::holds_alternative<std::string>(baseValue)) {
                std::string str = std::get<std::string>(baseValue);
                std::string suffix = std::get<std::string>(args[0]);
                if (suffix.length() <= str.length()) {
                  result = static_cast<bool>(
                      str.substr(str.length() - suffix.length()) == suffix);
                } else {
                  result = false;
                }
              } else {
                result = false; // Return false for non-string objects
              }
              return;
            } else if (methodName == "toUpperCase" && args.empty()) {
              // Handle toUpperCase() method call
              if (std::holds_alternative<std::string>(baseValue)) {
                std::string str = std::get<std::string>(baseValue);
                std::string upperStr = str;
                for (char &c : upperStr) {
                  c = static_cast<char>(
                      std::toupper(static_cast<unsigned char>(c)));
                }
                result = upperStr;
              } else {
                result = std::string(
                    ""); // Return empty string for non-string objects
              }
              return;
            } else if (methodName == "toLowerCase" && args.empty()) {
              // Handle toLowerCase() method call
              if (std::holds_alternative<std::string>(baseValue)) {
                std::string str = std::get<std::string>(baseValue);
                std::string lowerStr = str;
                for (char &c : lowerStr) {
                  c = static_cast<char>(
                      std::tolower(static_cast<unsigned char>(c)));
                }
                result = lowerStr;
              } else {
                result = std::string(
                    ""); // Return empty string for non-string objects
              }
              return;
            } else if (methodName == "trim" && args.empty()) {
              // Handle trim() method call
              if (std::holds_alternative<std::string>(baseValue)) {
                std::string str = std::get<std::string>(baseValue);
                // Left trim
                size_t start = str.find_first_not_of(" \t\n\r\f\v");
                if (start == std::string::npos) {
                  result = std::string(""); // String is all whitespace
                } else {
                  // Right trim
                  size_t end = str.find_last_not_of(" \t\n\r\f\v");
                  result = str.substr(start, end - start + 1);
                }
              } else {
                result = std::string(
                    ""); // Return empty string for non-string objects
              }
              return;
            } else if (methodName == "split" && args.size() == 1 &&
                       std::holds_alternative<std::string>(args[0])) {
              // Handle split(delimiter) method call
              if (std::holds_alternative<std::string>(baseValue)) {
                std::string str = std::get<std::string>(baseValue);
                std::string delimiter = std::get<std::string>(args[0]);

                std::vector<std::string> parts;
                size_t start = 0;
                size_t end = str.find(delimiter);

                while (end != std::string::npos) {
                  parts.push_back(str.substr(start, end - start));
                  start = end + delimiter.length();
                  end = str.find(delimiter, start);
                }

                parts.push_back(str.substr(start));

                // For now, return a string representation of the parts
                std::string resultStr = "[";
                for (size_t i = 0; i < parts.size(); ++i) {
                  resultStr += parts[i];
                  if (i < parts.size() - 1) {
                    resultStr += ", ";
                  }
                }
                resultStr += "]";
                result = resultStr;
              } else {
                result = std::string(
                    ""); // Return empty string for non-string objects
              }
              return;
            } else {
              // Unrecognized method on chained call, return default
              result = std::string("");
              return;
            }
          } else {
            // Handle general method calls on expressions (variables, literals,
            // etc.) - direct calls
            // methodName is already declared above, reuse it

            // Get the object value by evaluating the object expression
            // Safety check for null object
            if (!memberAccess->object) {
              std::cout << "ERROR: Direct method call has null object"
                        << std::endl;
              result = std::string("null_object_error");
              return;
            }
            // objValue is already declared above, reuse it
            objValue = interpreter->evaluate(*memberAccess->object);

            if (methodName == "toString" && args.empty()) {
              // Handle toString() method for all types
              result = interpreter->valueToString(objValue);
              return;
            } else if (methodName == "substring" && args.size() == 1 &&
                       std::holds_alternative<int>(args[0])) {
              // Handle substring(start) method call
              if (std::holds_alternative<std::string>(objValue)) {
                std::string str = std::get<std::string>(objValue);
                int start = std::get<int>(args[0]);
                if (start >= 0 && static_cast<size_t>(start) <= str.length()) {
                  result = str.substr(static_cast<size_t>(start));
                } else {
                  result =
                      std::string(""); // Return empty string for invalid index
                }
              }
              return;
            } else if (methodName == "substring" && args.size() == 2 &&
                       std::holds_alternative<int>(args[0]) &&
                       std::holds_alternative<int>(args[1])) {
              // Handle substring(start, end) method call
              if (std::holds_alternative<std::string>(objValue)) {
                std::string str = std::get<std::string>(objValue);
                int start = std::get<int>(args[0]);
                int end = std::get<int>(args[1]);
                if (start >= 0 && static_cast<size_t>(end) <= str.length() &&
                    start <= end) {
                  result = str.substr(static_cast<size_t>(start),
                                      static_cast<size_t>(end - start));
                } else {
                  result =
                      std::string(""); // Return empty string for invalid range
                }
              }
              return;
            } else if (methodName == "indexOf" && args.size() == 1 &&
                       std::holds_alternative<std::string>(args[0])) {
              // Handle indexOf(substring) method call
              if (std::holds_alternative<std::string>(objValue)) {
                std::string str = std::get<std::string>(objValue);
                std::string substr = std::get<std::string>(args[0]);
                size_t pos = str.find(substr);
                result = static_cast<int>(
                    pos != std::string::npos ? static_cast<int>(pos) : -1);
              } else {
                result = -1; // Return -1 for non-string objects
              }
              return;
            } else if (methodName == "startsWith" && args.size() == 1 &&
                       std::holds_alternative<std::string>(args[0])) {
              // Handle startsWith(prefix) method call
              if (std::holds_alternative<std::string>(objValue)) {
                std::string str = std::get<std::string>(objValue);
                std::string prefix = std::get<std::string>(args[0]);
                result =
                    static_cast<bool>(str.substr(0, prefix.length()) == prefix);
              } else {
                result = false; // Return false for non-string objects
              }
              return;
            } else if (methodName == "endsWith" && args.size() == 1 &&
                       std::holds_alternative<std::string>(args[0])) {
              // Handle endsWith(suffix) method call
              if (std::holds_alternative<std::string>(objValue)) {
                std::string str = std::get<std::string>(objValue);
                std::string suffix = std::get<std::string>(args[0]);
                if (suffix.length() <= str.length()) {
                  result = static_cast<bool>(
                      str.substr(str.length() - suffix.length()) == suffix);
                } else {
                  result = false;
                }
              }
              return;
            } else if (methodName == "toUpperCase" && args.empty()) {
              // Handle toUpperCase() method call
              if (std::holds_alternative<std::string>(objValue)) {
                std::string str = std::get<std::string>(objValue);
                std::string upperStr = str;
                for (char &c : upperStr) {
                  c = static_cast<char>(
                      std::toupper(static_cast<unsigned char>(c)));
                }
                result = upperStr;
              } else {
                result = std::string(
                    ""); // Return empty string for non-string objects
              }
              return;
            } else if (methodName == "toLowerCase" && args.empty()) {
              // Handle toLowerCase() method call
              if (std::holds_alternative<std::string>(objValue)) {
                std::string str = std::get<std::string>(objValue);
                std::string lowerStr = str;
                for (char &c : lowerStr) {
                  c = static_cast<char>(
                      std::tolower(static_cast<unsigned char>(c)));
                }
                result = lowerStr;
              } else {
                result = std::string(
                    ""); // Return empty string for non-string objects
              }
              return;
            } else if (methodName == "trim" && args.empty()) {
              // Handle trim() method call
              if (std::holds_alternative<std::string>(objValue)) {
                std::string str = std::get<std::string>(objValue);
                // Left trim
                size_t start = str.find_first_not_of(" \t\n\r\f\v");
                if (start == std::string::npos) {
                  result = std::string(""); // String is all whitespace
                } else {
                  // Right trim
                  size_t end = str.find_last_not_of(" \t\n\r\f\v");
                  result = str.substr(start, end - start + 1);
                }
              } else {
                result = std::string(
                    ""); // Return empty string for non-string objects
              }
              return;
            } else if (methodName == "split" && args.size() == 1 &&
                       std::holds_alternative<std::string>(args[0])) {
              // Handle split(delimiter) method call
              if (std::holds_alternative<std::string>(objValue)) {
                std::string str = std::get<std::string>(objValue);
                std::string delimiter = std::get<std::string>(args[0]);

                std::vector<std::string> parts;
                size_t start = 0;
                size_t end = str.find(delimiter);

                while (end != std::string::npos) {
                  parts.push_back(str.substr(start, end - start));
                  start = end + delimiter.length();
                  end = str.find(delimiter, start);
                }

                parts.push_back(str.substr(start));

                // For now, return a string representation of the parts
                std::string resultStr = "[";
                for (size_t i = 0; i < parts.size(); ++i) {
                  resultStr += parts[i];
                  if (i < parts.size() - 1) {
                    resultStr += ", ";
                  }
                }
                resultStr += "]";
                result = resultStr;
              } else {
                result = std::string(
                    ""); // Return empty string for non-string objects
              }
              return;
            } else if (std::holds_alternative<std::shared_ptr<ClassInstance>>(
                           objValue)) {
              // Handle method calls on class instances
              auto instance =
                  std::get<std::shared_ptr<ClassInstance>>(objValue);

              // Look up the method in the class definition
              for (const auto &method : instance->classDef->methods) {
                if (method->name == methodName) {
                  // Found the method, execute it
                  std::cout << "DEBUG: Found method '" << methodName
                            << "' in class '" << instance->className << "'"
                            << std::endl;
                  // Create new environment for method execution with 'this'
                  // bound to the instance
                  auto methodEnv =
                      std::make_shared<Environment>(interpreter->environment);

                  // Bind 'this' to the instance (for now, we'll use a special
                  // variable name)
                  methodEnv->define("this", Value(instance));
                  std::cout << "DEBUG: Defined 'this' in method environment"
                            << std::endl;

                  // Bind method parameters to arguments
                  for (size_t i = 0;
                       i < method->parameters.size() && i < args.size(); ++i) {
                    methodEnv->define(method->parameters[i].name, args[i]);
                  }

                  // Save current environment
                  auto prevEnv = interpreter->environment;
                  auto prevFuncEnv = interpreter->functionEnvironment;

                  // Switch to method environment
                  interpreter->environment = methodEnv;
                  interpreter->functionEnvironment = methodEnv;
                  std::cout << "DEBUG: Switched to method environment"
                            << std::endl;

                  // Execute method body
                  Value returnValue =
                      std::string("undefined"); // Default return value
                  try {
                    if (method->body) {
                      std::cout << "DEBUG: Executing method body" << std::endl;
                      if (auto *blockBody =
                              dynamic_cast<BlockStmt *>(method->body.get())) {
                        std::cout << "DEBUG: Method body is a block with "
                                  << blockBody->statements.size()
                                  << " statements" << std::endl;
                        for (const auto &stmt : blockBody->statements) {
                          if (stmt) {
                            std::cout << "DEBUG: Executing statement in method"
                                      << std::endl;
                            interpreter->execute(*stmt);
                          }
                        }
                      } else {
                        std::cout << "DEBUG: Method body is not a block"
                                  << std::endl;
                        interpreter->execute(*method->body);
                      }
                    } else {
                      std::cout << "DEBUG: Method body is null" << std::endl;
                    }
                  } catch (const std::runtime_error &e) {
                    std::string msg = e.what();
                    if (msg.substr(0, 7) == "RETURN:") {
                      std::string returnValStr = msg.substr(7);
                      // Parse return value based on type
                      // For now, just return as string
                      returnValue = returnValStr;
                    } else {
                      throw;
                    }
                  }

                  // Restore environment
                  interpreter->environment = prevEnv;
                  interpreter->functionEnvironment = prevFuncEnv;

                  result = returnValue;
                  return;
                }
              }

              // Method not found
              std::cout << "Error: Method '" << methodName
                        << "' not found in class " << instance->className
                        << std::endl;
              result = std::string("method_not_found:" + methodName);
              return;
            } else {
              // Unrecognized method on direct call, return default
              result = std::string("");
              return;
            }
          }
        }
      }
      // If we reach here and the callee is not a MemberAccessExpr, it means we
      // need to handle chained method calls differently. Actually, the
      // structure is correct: When we have expr.method().otherMethod(), the
      // outer call is otherMethod() on the result of expr.method(), so the
      // callee of the outer call is the MemberAccessExpr for .otherMethod() The
      // original structure was correct but had a syntax error. We should check
      // for non-MemberAccess call expressions after the main MemberAccess
      // handling.

      // Check if it's a built-in function like println
      if (auto *ident = dynamic_cast<IdentifierExpr *>(node.callee.get())) {
        if (ident->name == "println") {
          if (!args.empty()) {
            // Convert the argument to string and print it
            std::string output = interpreter->valueToString(args[0]);
            std::cout << output << std::endl;
          } else {
            std::cout << std::endl;
          }
          result = 0; // Return a default value
          return;
        } else if (ident->name == "print") {
          if (!args.empty()) {
            // Convert the argument to string and print it without newline
            std::string output = interpreter->valueToString(args[0]);
            std::cout << output;
          }
          result = 0; // Return a default value
          return;
        } else if (ident->name == "readln") {
          // Read a line from standard input
          std::string input;
          std::getline(std::cin, input);
          result = input;
          return;
        } else if (ident->name == "printf" || ident->name == "format") {
          // Handle printf-style formatting
          if (!args.empty()) {
            // First argument is the format string
            std::string formatStr = interpreter->valueToString(args[0]);

            // For now, implement basic string replacement
            std::string resultStr = formatStr;

            // Replace placeholders like %s, %d, %f with actual values
            size_t argIndex = 1;
            for (size_t i = 0; i < resultStr.length() && argIndex < args.size();
                 ++i) {
              if (resultStr[i] == '%' && i + 1 < resultStr.length()) {
                char nextChar = resultStr[i + 1];
                if (nextChar == 's' || nextChar == 'd' || nextChar == 'f' ||
                    nextChar == 'i') {
                  std::string replacement =
                      interpreter->valueToString(args[argIndex]);
                  resultStr.replace(i, 2, replacement);
                  ++argIndex;
                  i += replacement.length() -
                       1; // Adjust position after replacement
                }
              }
            }

            result = resultStr;
          } else {
            result = std::string("");
          }
          return;
        } else if (ident->name == "toInt") {
          // Convert string to integer
          if (!args.empty()) {
            std::string str = interpreter->valueToString(args[0]);
            try {
              int val = std::stoi(str);
              result = val;
            } catch (...) {
              result = 0; // Return 0 on conversion error
            }
          } else {
            result = 0;
          }
          return;
        } else if (ident->name == "toDouble") {
          // Convert string to double
          if (!args.empty()) {
            std::string str = interpreter->valueToString(args[0]);
            try {
              double val = std::stod(str);
              result = val;
            } catch (...) {
              result = 0.0; // Return 0.0 on conversion error
            }
          } else {
            result = 0.0;
          }
          return;
        } else if (ident->name == "arrayOf") {
          // Create an array from the arguments
          result = ArrayValue(std::vector<Value>(args.begin(), args.end()));
          return;
        } else if (ident->name == "intArrayOf") {
          // Create an integer array from the arguments (convert all to
          // integers)
          std::vector<Value> intElements;
          for (const auto &arg : args) {
            if (std::holds_alternative<int>(arg)) {
              intElements.push_back(arg);
            } else if (std::holds_alternative<double>(arg)) {
              intElements.push_back(static_cast<int>(std::get<double>(arg)));
            } else if (std::holds_alternative<std::string>(arg)) {
              try {
                int val = std::stoi(std::get<std::string>(arg));
                intElements.push_back(val);
              } catch (...) {
                intElements.push_back(0); // default value
              }
            } else {
              intElements.push_back(0); // default value
            }
          }
          result = ArrayValue(std::move(intElements));
          return;
        } else if (ident->name == "doubleArrayOf") {
          // Create a double array from the arguments (convert all to doubles)
          std::vector<Value> doubleElements;
          for (const auto &arg : args) {
            if (std::holds_alternative<double>(arg)) {
              doubleElements.push_back(arg);
            } else if (std::holds_alternative<int>(arg)) {
              doubleElements.push_back(static_cast<double>(std::get<int>(arg)));
            } else if (std::holds_alternative<std::string>(arg)) {
              try {
                double val = std::stod(std::get<std::string>(arg));
                doubleElements.push_back(val);
              } catch (...) {
                doubleElements.push_back(0.0); // default value
              }
            } else {
              doubleElements.push_back(0.0); // default value
            }
          }
          result = ArrayValue(std::move(doubleElements));
          return;
        } else if (ident->name == "stringArrayOf") {
          // Create a string array from the arguments (convert all to strings)
          std::vector<Value> stringElements;
          for (const auto &arg : args) {
            stringElements.push_back(interpreter->valueToString(arg));
          }
          result = ArrayValue(std::move(stringElements));
          return;
        } else if (ident->name == "throw") {
          // Throw an exception with the provided message
          std::string errorMsg = "Runtime error";
          if (!args.empty()) {
            errorMsg = interpreter->valueToString(args[0]);
          }
          throw std::runtime_error(errorMsg);
        }
      }

      // First, try to evaluate the callee to handle complex expressions (like
      // lambda calls)
      Value calleeValue = interpreter->evaluate(*node.callee);

      // Check if the callee is a lambda function
      if (std::holds_alternative<std::shared_ptr<LambdaValue>>(calleeValue)) {
        auto lambda = std::get<std::shared_ptr<LambdaValue>>(calleeValue);

        // Create new environment for lambda execution with closure
        auto lambdaEnv = std::make_shared<Environment>(lambda->closure);

        // Bind parameters to arguments
        for (size_t i = 0; i < lambda->parameters.size() && i < args.size();
             ++i) {
          lambdaEnv->define(lambda->parameters[i].name, args[i]);
        }

        // Save current environment
        auto prevEnv = interpreter->environment;
        auto prevFuncEnv = interpreter->functionEnvironment;

        // Switch to lambda environment
        interpreter->environment = lambdaEnv;
        interpreter->functionEnvironment = lambdaEnv;

        // Execute lambda body
        Value returnValue = std::string("undefined"); // Default return value
        try {
          if (lambda->body) {
            if (auto *blockBody =
                    dynamic_cast<BlockStmt *>(lambda->body.get())) {
              for (const auto &stmt : blockBody->statements) {
                if (stmt)
                  interpreter->execute(*stmt);
              }
            } else {
              interpreter->execute(*lambda->body);
            }
          }
          // If lambda body is null but original_node exists, execute the
          // original lambda body
          else if (lambda->original_node && lambda->original_node->body) {
            if (auto *blockBody = dynamic_cast<BlockStmt *>(
                    lambda->original_node->body.get())) {
              // For block statements, execute all but the last as statements
              // and evaluate the last as expression
              if (!blockBody->statements.empty()) {
                for (size_t i = 0; i < blockBody->statements.size() - 1; ++i) {
                  interpreter->execute(*blockBody->statements[i]);
                }
                // Evaluate the last statement as an expression and return its
                // value
                auto &lastStmt = blockBody->statements.back();
                if (lastStmt) {
                  // If it's an expression statement, evaluate the expression
                  if (auto *exprStmt =
                          dynamic_cast<ExpressionStmt *>(lastStmt.get())) {
                    returnValue = interpreter->evaluate(*exprStmt->expression);
                  } else {
                    // For other types of statements, execute them but this may
                    // not return a value
                    interpreter->execute(*lastStmt);
                  }
                }
              }
            } else {
              // If the body is a single expression (not in a block), evaluate
              // it as an expression First, check if it's an expression
              // statement
              if (auto *exprStmt = dynamic_cast<ExpressionStmt *>(
                      lambda->original_node->body.get())) {
                returnValue = interpreter->evaluate(*exprStmt->expression);
              } else {
                // For non-expression statements, just execute them
                interpreter->execute(*lambda->original_node->body);
              }
            }
          }
        } catch (const std::runtime_error &e) {
          // Check if this is a return value wrapped in an exception
          std::string msg = e.what();
          if (msg.substr(0, 7) == "RETURN:") {
            // Extract the return value
            std::string returnValStr = msg.substr(7);
            // Try to parse as integer first, then as double, then as string
            // Check if the string contains a decimal point to determine if it's
            // an int or double
            size_t dotPos = returnValStr.find('.');
            if (dotPos != std::string::npos) {
              // Contains decimal point, parse as double
              char *end;
              double value = strtod(returnValStr.c_str(), &end);
              if (*end == 0) { // successfully parsed as a number
                returnValue = Value(value);
              } else {
                // Not a valid number, treat as string
                returnValue = Value(returnValStr);
              }
            } else {
              // No decimal point, parse as integer
              char *end;
              long value = strtol(returnValStr.c_str(), &end, 10);
              if (*end == 0) { // successfully parsed as an integer
                returnValue = Value(static_cast<int>(value));
              } else {
                // Not a valid number, treat as string
                returnValue = Value(returnValStr);
              }
            }
          } else {
            // Re-throw non-return exceptions
            throw;
          }
        }

        // Restore environment
        interpreter->environment = prevEnv;
        interpreter->functionEnvironment = prevFuncEnv;

        result = returnValue;
        return;
      }

      // Check if this is a user-defined function call
      auto *identifier = dynamic_cast<IdentifierExpr *>(node.callee.get());
      if (identifier) {
        // First, check if this is a class instantiation
        Value classValue;
        try {
          classValue = interpreter->environment->get(identifier->name);
          if (std::holds_alternative<std::shared_ptr<ClassDefinition>>(
                  classValue)) {
            // This is a class instantiation - we need to find and execute the
            // constructor
            auto classDef =
                std::get<std::shared_ptr<ClassDefinition>>(classValue);

            // Find a matching constructor based on argument count
            std::shared_ptr<FunctionDef> selectedConstructor = nullptr;
            for (auto &ctor : classDef->constructors) {
              if (ctor->parameters.size() == args.size()) {
                selectedConstructor = ctor;
                break;
              }
            }

            if (selectedConstructor) {
              // Create a new class instance
              auto instance =
                  std::make_shared<ClassInstance>(classDef->name, classDef);

              // Execute constructor body with 'this' bound to the instance
              if (selectedConstructor->body) {
                // Create environment for constructor execution
                auto constructorEnv =
                    std::make_shared<Environment>(interpreter->environment);

                // Bind 'this' to the instance
                constructorEnv->define("this", Value(instance));

                // Bind constructor parameters to arguments
                for (size_t i = 0; i < selectedConstructor->parameters.size() &&
                                   i < args.size();
                     ++i) {
                  constructorEnv->define(
                      selectedConstructor->parameters[i].name, args[i]);
                }

                // Save current environment
                auto prevEnv = interpreter->environment;
                auto prevFuncEnv = interpreter->functionEnvironment;

                // Switch to constructor environment
                interpreter->environment = constructorEnv;
                interpreter->functionEnvironment = constructorEnv;

                // Execute constructor body
                try {
                  if (auto *blockBody = dynamic_cast<BlockStmt *>(
                          selectedConstructor->body.get())) {
                    for (const auto &stmt : blockBody->statements) {
                      if (stmt) {
                        interpreter->execute(*stmt);
                      }
                    }
                  } else {
                    interpreter->execute(*selectedConstructor->body);
                  }
                } catch (const std::runtime_error &e) {
                  std::string msg = e.what();
                  if (msg.substr(0, 7) != "RETURN:") {
                    // Re-throw non-return exceptions
                    throw;
                  }
                  // Ignore return statements in constructors
                }

                // Restore environment
                interpreter->environment = prevEnv;
                interpreter->functionEnvironment = prevFuncEnv;
              }

              result = instance;
              return;
            } else {
              std::cout << "Error: No matching constructor found for "
                        << classDef->name << " with " << args.size()
                        << " arguments" << std::endl;
              result = std::string("constructor_not_found");
              return;
            }
          }
        } catch (const std::exception &) {
          // Not a class, continue with function call logic
        }

        auto selectedFunc =
            interpreter->findBestFunctionOverload(identifier->name, args);

        if (selectedFunc) {
          // Found matching function overload, execute it

          // Create new environment for function execution
          auto funcEnv =
              std::make_shared<Environment>(interpreter->environment);

          // Bind parameters to arguments
          for (size_t i = 0;
               i < selectedFunc->parameters.size() && i < args.size(); ++i) {
            funcEnv->define(selectedFunc->parameters[i].name, args[i]);
          }

          // Save current environment
          auto prevEnv = interpreter->environment;
          auto prevFuncEnv = interpreter->functionEnvironment;

          // Switch to function environment
          interpreter->environment = funcEnv;
          interpreter->functionEnvironment = funcEnv;

          // Execute function body
          Value returnValue = std::string("undefined"); // Default return value
          try {
            if (selectedFunc->body) {
              if (auto *blockBody =
                      dynamic_cast<BlockStmt *>(selectedFunc->body.get())) {
                for (const auto &stmt : blockBody->statements) {
                  if (stmt)
                    interpreter->execute(*stmt);
                }
              } else {
                interpreter->execute(*selectedFunc->body);
              }
            }
          } catch (const std::runtime_error &e) {
            // Check if this is a return value wrapped in an exception
            std::string msg = e.what();
            if (msg.substr(0, 7) == "RETURN:") {
              // Extract the return value
              std::string returnValStr = msg.substr(7);
              // Try to parse as integer first, then as double, then as string
              // Check if the string contains a decimal point to determine if
              // it's an int or double
              size_t dotPos = returnValStr.find('.');
              if (dotPos != std::string::npos) {
                // Contains decimal point, parse as double
                char *end;
                double value = strtod(returnValStr.c_str(), &end);
                if (*end == 0) { // successfully parsed as a number
                  returnValue = Value(value);
                } else {
                  // Not a valid number, treat as string
                  returnValue = Value(returnValStr);
                }
              } else {
                // No decimal point, parse as integer
                char *end;
                long value = strtol(returnValStr.c_str(), &end, 10);
                if (*end == 0) { // successfully parsed as an integer
                  returnValue = Value(static_cast<int>(value));
                } else {
                  // Not a valid number, treat as string
                  returnValue = Value(returnValStr);
                }
              }
            } else {
              // Re-throw non-return exceptions
              throw;
            }
          }

          // Restore environment
          interpreter->environment = prevEnv;
          interpreter->functionEnvironment = prevFuncEnv;

          result = returnValue;
          return;
        }

        // No matching function found
        std::cout << "Error: No matching function overload found for "
                  << identifier->name << " with " << args.size() << " arguments"
                  << std::endl;
        result = std::string("call_result");
        return;
      }

      // For other complex expressions as callee, return a placeholder
      result = std::string("complex_call_result");
    }

    void visit(MemberAccessExpr &node) override {
      // Handle member access
      if (!node.object) {
        std::cout << "ERROR: Member access expression has null object"
                  << std::endl;
        result = std::string("null_object_error");
        return;
      }

      // Debug: Print object type
      if (auto *identifier =
              dynamic_cast<IdentifierExpr *>(node.object.get())) {
        std::cout << "DEBUG: Member access on identifier '" << identifier->name
                  << "' property '" << node.property << "'" << std::endl;
      }

      auto objValue = node.object ? interpreter->evaluate(*node.object)
                                  : Value(std::string("null"));

      // Debug: Print if object evaluation succeeded
      if (auto *identifier =
              dynamic_cast<IdentifierExpr *>(node.object.get())) {
        std::cout << "DEBUG: Evaluated object '" << identifier->name << "'"
                  << std::endl;
      }

      // Check if the object is an identifier "args" and the property is "size"
      // or "contentToString"
      if (auto *identifier =
              dynamic_cast<IdentifierExpr *>(node.object.get())) {
        if (identifier->name == "args" && node.property == "size") {
          // Return the size of command-line arguments
          result = static_cast<int>(interpreter->commandLineArgs.size());
          return;
        } else if (identifier->name == "args" &&
                   node.property == "contentToString") {
          // Return a string representation of all command-line arguments
          std::string content = "[";
          for (size_t i = 0; i < interpreter->commandLineArgs.size(); ++i) {
            content += interpreter->commandLineArgs[i];
            if (i < interpreter->commandLineArgs.size() - 1) {
              content += ", ";
            }
          }
          content += "]";
          result = content;
          return;
        } else if (identifier->name == "this") {
          // Handle 'this' field access in methods
          try {
            Value thisValue = interpreter->environment->get("this");
            if (std::holds_alternative<std::shared_ptr<ClassInstance>>(
                    thisValue)) {
              auto instance =
                  std::get<std::shared_ptr<ClassInstance>>(thisValue);
              auto it = instance->fields.find(node.property);
              if (it != instance->fields.end()) {
                result = it->second;
              } else {
                result = std::string("field_not_found:" + node.property);
              }
              return;
            }
          } catch (const std::exception &) {
            result = std::string("this_not_available");
            return;
          }
        }
      }

      if (node.property == "size" &&
          std::holds_alternative<ArrayValue>(objValue)) {
        // Handle .size property for arrays
        const ArrayValue &arr = std::get<ArrayValue>(objValue);
        result = static_cast<int>(arr.elements.size());
      } else if (std::holds_alternative<std::shared_ptr<ClassInstance>>(
                     objValue)) {
        // Handle property access for class instances
        auto instance = std::get<std::shared_ptr<ClassInstance>>(objValue);
        auto it = instance->fields.find(node.property);
        if (it != instance->fields.end()) {
          result = it->second;
        } else {
          // Property not found, return error
          result = std::string("property_not_found: " + node.property);
        }
        return;
      } else if (node.property == "toString") {
        // Handle .toString() method for all types
        result = interpreter->valueToString(objValue);
      } else if (node.property == "substring" &&
                 std::holds_alternative<std::string>(objValue)) {
        // Handle .substring() property access without arguments
        // For now, return a placeholder - substring with arguments is handled
        // in CallExpr
        result = std::string("");
      } else if (node.property == "indexOf" &&
                 std::holds_alternative<std::string>(objValue)) {
        // Handle .indexOf property access without arguments
        // For now, return a placeholder - indexOf with arguments is handled in
        // CallExpr
        result = std::string("");
      } else if (node.property == "startsWith" &&
                 std::holds_alternative<std::string>(objValue)) {
        // Handle .startsWith property access without arguments
        // For now, return a placeholder - startsWith with arguments is handled
        // in CallExpr
        result = std::string("");
      } else if (node.property == "endsWith" &&
                 std::holds_alternative<std::string>(objValue)) {
        // Handle .endsWith property access without arguments
        // For now, return a placeholder - endsWith with arguments is handled in
        // CallExpr
        result = std::string("");
      } else if (node.property == "toUpperCase" &&
                 std::holds_alternative<std::string>(objValue)) {
        // Handle .toUpperCase property access without arguments
        // For now, return a placeholder - toUpperCase with arguments is handled
        // in CallExpr
        result = std::string("");
      } else if (node.property == "toLowerCase" &&
                 std::holds_alternative<std::string>(objValue)) {
        // Handle .toLowerCase property access without arguments
        // For now, return a placeholder - toLowerCase with arguments is handled
        // in CallExpr
        result = std::string("");
      } else if (node.property == "trim" &&
                 std::holds_alternative<std::string>(objValue)) {
        // Handle .trim property access without arguments
        // For now, return a placeholder - trim with arguments is handled in
        // CallExpr
        result = std::string("");
      } else if (node.property == "split" &&
                 std::holds_alternative<std::string>(objValue)) {
        // Handle .split property access without arguments
        // For now, return a placeholder - split with arguments is handled in
        // CallExpr
        result = std::string("");
      } else if (std::holds_alternative<std::shared_ptr<ClassInstance>>(
                     objValue)) {
        // Handle member access on class instances (field access)
        auto instance = std::get<std::shared_ptr<ClassInstance>>(objValue);

        // Try to find the field in the instance
        auto fieldIt = instance->fields.find(node.property);
        if (fieldIt != instance->fields.end()) {
          // Found field, return its value
          result = fieldIt->second;
        } else {
          // Property not found as field, return a placeholder
          result = std::string("field_not_found:" + node.property);
        }
      } else {
        // For now, return a placeholder for other member accesses
        // Debug: check what type of object we have
        std::cout << "DEBUG: Member access on property '" << node.property
                  << "' with object type: ";
        if (std::holds_alternative<std::shared_ptr<ClassInstance>>(objValue)) {
          std::cout << "ClassInstance";
        } else if (std::holds_alternative<std::string>(objValue)) {
          std::cout << "String";
        } else if (std::holds_alternative<ArrayValue>(objValue)) {
          std::cout << "ArrayValue";
        } else {
          std::cout << "Other";
        }
        std::cout << std::endl;
        result = std::string("member_access_result");
      }
    }

    void visit(ArrayAccessExpr &node) override {
      // Handle array access like args[0] or myArray[1]
      if (!node.array || !node.index) {
        std::cout << "ERROR: Array access expression has null array or index"
                  << std::endl;
        result = std::string("null_access_error");
        return;
      }
      auto arrayValue = node.array ? interpreter->evaluate(*node.array)
                                   : Value(std::string("null"));
      auto indexValue = node.index ? interpreter->evaluate(*node.index)
                                   : Value(std::string("null"));

      // Check if we're accessing the args array (legacy handling)
      if (std::holds_alternative<std::string>(arrayValue) &&
          std::get<std::string>(arrayValue) == "args_array" &&
          std::holds_alternative<int>(indexValue)) {
        int index = std::get<int>(indexValue);
        if (index >= 0 &&
            static_cast<size_t>(index) < interpreter->commandLineArgs.size()) {
          result = interpreter->commandLineArgs[static_cast<size_t>(index)];
        } else {
          result = std::string("index_out_of_bounds");
        }
      }
      // Handle access to actual ArrayValue
      else if (std::holds_alternative<ArrayValue>(arrayValue) &&
               std::holds_alternative<int>(indexValue)) {
        const ArrayValue &arr = std::get<ArrayValue>(arrayValue);
        int index = std::get<int>(indexValue);

        if (index >= 0 && static_cast<size_t>(index) < arr.elements.size()) {
          result = arr.elements[static_cast<size_t>(index)];
        } else {
          result = std::string("index_out_of_bounds");
        }
      } else {
        result = std::string("unsupported_array_access");
      }
    }

    void visit(ArrayLiteralExpr &node) override {
      // Evaluate each element in the array literal
      std::vector<Value> elements;
      for (auto &element : node.elements) {
        if (element) {
          elements.push_back(interpreter->evaluate(*element));
        } else {
          elements.push_back(Value(std::string("undefined")));
        }
      }
      // Create an ArrayValue with the evaluated elements
      result = ArrayValue(std::move(elements));
    }

    // Statement visitor methods (required by interface but shouldn't be called
    // for expressions)
    void visit(ExpressionStmt &node) override {
      result = std::string("error: expression visitor called on statement");
      (void)node;
    }
    void visit(VariableDeclStmt &node) override {
      result = std::string("error: expression visitor called on statement");
      (void)node;
    }
    void visit(FunctionDeclStmt &node) override {
      result = std::string("error: expression visitor called on statement");
      (void)node;
    }
    void visit(BlockStmt &node) override {
      result = std::string("error: expression visitor called on statement");
      (void)node;
    }
    void visit(ReturnStmt &node) override {
      result = std::string("error: expression visitor called on statement");
      (void)node;
    }
    void visit(IfStmt &node) override {
      result = std::string("error: expression visitor called on statement");
      (void)node;
    }
    void visit(WhileStmt &node) override {
      result = std::string("error: expression visitor called on statement");
      (void)node;
    }
    void visit(ForStmt &node) override {
      result = std::string("error: expression visitor called on statement");
      (void)node;
    }
    void visit(WhenStmt &node) override {
      result = std::string("error: expression visitor called on statement");
      (void)node;
    }
    void visit(TryStmt &node) override {
      result = std::string("error: expression visitor called on statement");
      (void)node;
    }
    void visit(ClassDeclStmt &node) override {
      result = std::string("error: expression visitor called on statement");
      (void)node;
    }
    void visit(ConstructorDeclStmt &node) override {
      result = std::string("error: expression visitor called on statement");
      (void)node;
    }
  };

  EvalVisitor visitor(this);
  try {
    expr.accept(visitor);
  } catch (const std::exception &e) {
    std::cout << "Exception in expression evaluation: " << e.what()
              << std::endl;
    // Return a default value in case of error
    evaluationDepth--; // Decrement before returning
    return std::string("evaluation_error");
  } catch (...) {
    std::cout << "Unknown exception in expression evaluation" << std::endl;
    evaluationDepth--; // Decrement before returning
    return std::string("evaluation_error");
  }
  // Decrement evaluation depth before returning
  evaluationDepth--;
  return visitor.result;
}

void Interpreter::execute(Statement &stmt) {
  // Use the visitor pattern to dispatch to the appropriate visit method
  struct ExecVisitor : public AstVisitor {
    Interpreter *interpreter;

    ExecVisitor(Interpreter *interp) : interpreter(interp) {}

    void visit(LiteralExpr &node) override { (void)node; }
    void visit(IdentifierExpr &node) override { (void)node; }
    void visit(BinaryExpr &node) override { (void)node; }
    void visit(UnaryExpr &node) override { (void)node; }
    void visit(CallExpr &node) override {
      // Handle function calls for side effects (like println)
      interpreter->visit(node);
    }
    void visit(MemberAccessExpr &node) override {
      // Evaluate the member access expression
      interpreter->evaluate(node);
    }
    void visit(ArrayAccessExpr &node) override {
      // Evaluate the array access expression
      interpreter->evaluate(node);
    }
    void visit(ArrayLiteralExpr &node) override {
      // Evaluate the array literal expression
      interpreter->evaluate(node);
    }
    void visit(LambdaExpr &node) override { (void)node; }
    void visit(ExpressionStmt &node) override { interpreter->visit(node); }
    void visit(VariableDeclStmt &node) override { interpreter->visit(node); }
    void visit(FunctionDeclStmt &node) override { interpreter->visit(node); }
    void visit(BlockStmt &node) override { interpreter->visit(node); }
    void visit(ReturnStmt &node) override { interpreter->visit(node); }
    void visit(IfStmt &node) override { interpreter->visit(node); }
    void visit(WhileStmt &node) override { interpreter->visit(node); }
    void visit(ForStmt &node) override { interpreter->visit(node); }
    void visit(WhenStmt &node) override { interpreter->visit(node); }
    void visit(TryStmt &node) override { interpreter->visit(node); }
    void visit(ConstructorDeclStmt &node) override {
      (void)node; /* For now, just acknowledge the constructor */
    }
    void visit(ClassDeclStmt &node) override { interpreter->visit(node); }
  };

  ExecVisitor visitor(this);
  stmt.accept(visitor);
}

std::shared_ptr<Type> Interpreter::getTypeOfValue(const Value &value) {
  if (std::holds_alternative<int>(value)) {
    return std::make_shared<Type>(TypeKind::INT);
  } else if (std::holds_alternative<double>(value)) {
    return std::make_shared<Type>(TypeKind::DOUBLE);
  } else if (std::holds_alternative<bool>(value)) {
    return std::make_shared<Type>(TypeKind::BOOL);
  } else if (std::holds_alternative<std::string>(value)) {
    return std::make_shared<Type>(TypeKind::STRING);
  } else if (std::holds_alternative<ArrayValue>(value)) {
    return std::make_shared<Type>(TypeKind::ARRAY);
  } else if (std::holds_alternative<std::shared_ptr<ClassInstance>>(value)) {
    return std::make_shared<Type>(TypeKind::UNKNOWN);
  } else {
    return std::make_shared<Type>(TypeKind::UNKNOWN);
  }
}

std::string Interpreter::typeToString(const std::shared_ptr<Type> &type) {
  if (!type)
    return "unknown";
  switch (type->kind) {
  case TypeKind::INT:
    return "Int";
  case TypeKind::DOUBLE:
    return "Double";
  case TypeKind::BOOL:
    return "Boolean";
  case TypeKind::STRING:
    return "String";
  case TypeKind::ARRAY:
    return "Array";
  case TypeKind::VOID:
    return "Unit";
  case TypeKind::UNKNOWN:
    return "Unknown";
  case TypeKind::ANY:
    return "Any";
  default:
    return "unknown";
  }
}

std::string Interpreter::valueToString(const Value &value) {
  if (std::holds_alternative<int>(value)) {
    return std::to_string(std::get<int>(value));
  } else if (std::holds_alternative<double>(value)) {
    return std::to_string(std::get<double>(value));
  } else if (std::holds_alternative<bool>(value)) {
    return std::get<bool>(value) ? "true" : "false";
  } else if (std::holds_alternative<std::string>(value)) {
    return std::get<std::string>(value);
  } else if (std::holds_alternative<ArrayValue>(value)) {
    std::string result = "[";
    const ArrayValue &arr = std::get<ArrayValue>(value);
    for (size_t i = 0; i < arr.elements.size(); ++i) {
      result += valueToString(arr.elements[i]);
      if (i < arr.elements.size() - 1) {
        result += ", ";
      }
    }
    result += "]";
    return result;
  } else if (std::holds_alternative<std::shared_ptr<ClassInstance>>(value)) {
    auto instance = std::get<std::shared_ptr<ClassInstance>>(value);
    return instance->className + " instance";
  } else {
    return "undefined";
  }
}

// Type checker implementation
class TypeChecker {
public:
  std::shared_ptr<Environment> environment;

  TypeChecker(std::shared_ptr<Environment> env) : environment(env) {}

  // Helper method to get type of a value
  std::shared_ptr<dotlin::Type> getTypeOfValue(const Value &value) {
    if (std::holds_alternative<int>(value)) {
      return std::make_shared<dotlin::Type>(dotlin::TypeKind::INT);
    } else if (std::holds_alternative<double>(value)) {
      return std::make_shared<dotlin::Type>(dotlin::TypeKind::DOUBLE);
    } else if (std::holds_alternative<bool>(value)) {
      return std::make_shared<dotlin::Type>(dotlin::TypeKind::BOOL);
    } else if (std::holds_alternative<std::string>(value)) {
      return std::make_shared<dotlin::Type>(dotlin::TypeKind::STRING);
    } else if (std::holds_alternative<ArrayValue>(value)) {
      auto arrayValue = std::get<ArrayValue>(value);
      std::shared_ptr<dotlin::Type> elementType = nullptr;
      if (!arrayValue.elements.empty()) {
        elementType = getTypeOfValue(arrayValue.elements[0]);
      }
      return std::make_shared<dotlin::Type>(dotlin::TypeKind::ARRAY,
                                            elementType);
    } else if (std::holds_alternative<std::shared_ptr<ClassInstance>>(value)) {
      return std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
    } else {
      return std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
    }
  }

  // Helper method to convert type to string
  std::string typeToString(const std::shared_ptr<dotlin::Type> &type) {
    if (!type)
      return "unknown";
    switch (type->kind) {
    case dotlin::TypeKind::INT:
      return "Int";
    case dotlin::TypeKind::DOUBLE:
      return "Double";
    case dotlin::TypeKind::BOOL:
      return "Boolean";
    case dotlin::TypeKind::STRING:
      return "String";
    case dotlin::TypeKind::ARRAY:
      if (type->elementType) {
        return "Array<" + typeToString(type->elementType) + ">";
      }
      return "Array";
    case dotlin::TypeKind::VOID:
      return "Unit";
    case dotlin::TypeKind::UNKNOWN:
      return "Unknown";
    case dotlin::TypeKind::ANY:
      return "Any";
    default:
      return "unknown";
    }
  }

  std::shared_ptr<dotlin::Type> checkExpression(dotlin::Expression &expr) {
    (void)expr; // Suppress unused parameter warning
    // Use visitor pattern to check expression types
    struct TypeCheckVisitor : public dotlin::AstVisitor {
      std::shared_ptr<dotlin::Type> result;
      TypeChecker *checker;

      TypeCheckVisitor(TypeChecker *chk) : checker(chk) {}

      void visit(dotlin::LiteralExpr &node) override {
        // Determine type from literal value
        if (std::holds_alternative<int>(node.value)) {
          result = std::make_shared<dotlin::Type>(dotlin::TypeKind::INT);
        } else if (std::holds_alternative<double>(node.value)) {
          result = std::make_shared<dotlin::Type>(dotlin::TypeKind::DOUBLE);
        } else if (std::holds_alternative<bool>(node.value)) {
          result = std::make_shared<dotlin::Type>(dotlin::TypeKind::BOOL);
        } else if (std::holds_alternative<std::string>(node.value)) {
          result = std::make_shared<dotlin::Type>(dotlin::TypeKind::STRING);
        } else {
          result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
        }
      }

      void visit(dotlin::IdentifierExpr &node) override {
        // Look up the type of the identifier in the environment
        // Try to get the value and infer its type
        try {
          auto value = checker->environment->get(node.name);
          result = checker->getTypeOfValue(value);
        } catch (const std::exception &) {
          // Variable not found in environment
          result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
        }
      }

      void visit(dotlin::BinaryExpr &node) override {
        auto leftType = checker->checkExpression(*node.left);
        auto rightType = checker->checkExpression(*node.right);

        // Check type compatibility based on operator
        switch (node.op) {
        case dotlin::TokenType::PLUS:
          // Addition: both operands should be numeric or one string
          if (leftType->kind == dotlin::TypeKind::STRING ||
              rightType->kind == dotlin::TypeKind::STRING) {
            result = std::make_shared<dotlin::Type>(dotlin::TypeKind::STRING);
          } else if (leftType->isCompatibleWith(*rightType)) {
            result = std::make_shared<dotlin::Type>(leftType->kind);
          } else {
            std::cout << "Type error: incompatible types for + operator"
                      << std::endl;
            result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
          }
          break;
        case dotlin::TokenType::MINUS:
        case dotlin::TokenType::MULTIPLY:
        case dotlin::TokenType::DIVIDE:
          // Arithmetic operations require numeric types
          if (leftType->kind == dotlin::TypeKind::INT ||
              leftType->kind == dotlin::TypeKind::DOUBLE) {
            if (rightType->kind == dotlin::TypeKind::INT ||
                rightType->kind == dotlin::TypeKind::DOUBLE) {
              // Result is the higher precision type
              if (leftType->kind == dotlin::TypeKind::DOUBLE ||
                  rightType->kind == dotlin::TypeKind::DOUBLE) {
                result =
                    std::make_shared<dotlin::Type>(dotlin::TypeKind::DOUBLE);
              } else {
                result = std::make_shared<dotlin::Type>(dotlin::TypeKind::INT);
              }
            } else {
              std::cout << "Type error: right operand must be numeric for "
                           "arithmetic operator"
                        << std::endl;
              result =
                  std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
            }
          } else {
            std::cout << "Type error: left operand must be numeric for "
                         "arithmetic operator"
                      << std::endl;
            result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
          }
          break;
        case dotlin::TokenType::EQUAL:
        case dotlin::TokenType::NOT_EQUAL:
          // Comparison operators return boolean
          if (leftType->isCompatibleWith(*rightType)) {
            result = std::make_shared<dotlin::Type>(dotlin::TypeKind::BOOL);
          } else {
            std::cout
                << "Type error: incompatible types for comparison operator"
                << std::endl;
            result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
          }
          break;
        case dotlin::TokenType::LESS:
        case dotlin::TokenType::LESS_EQUAL:
        case dotlin::TokenType::GREATER:
        case dotlin::TokenType::GREATER_EQUAL:
          // Relational operators return boolean, require comparable types
          if (leftType->isCompatibleWith(*rightType)) {
            result = std::make_shared<dotlin::Type>(dotlin::TypeKind::BOOL);
          } else {
            std::cout
                << "Type error: incompatible types for relational operator"
                << std::endl;
            result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
          }
          break;
        case dotlin::TokenType::AND:
        case dotlin::TokenType::OR:
          // Logical operators require boolean types
          if (leftType->kind == dotlin::TypeKind::BOOL &&
              rightType->kind == dotlin::TypeKind::BOOL) {
            result = std::make_shared<dotlin::Type>(dotlin::TypeKind::BOOL);
          } else {
            std::cout
                << "Type error: logical operators require boolean operands"
                << std::endl;
            result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
          }
          break;
        case dotlin::TokenType::ASSIGN:
          // Assignment: right operand type should be compatible with left
          if (rightType->isCompatibleWith(*leftType)) {
            result = rightType; // Assignment returns the assigned value type
          } else {
            std::cout << "Type error: incompatible types for assignment"
                      << std::endl;
            result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
          }
          break;
        default:
          result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
          break;
        }
      }

      void visit(dotlin::UnaryExpr &node) override {
        auto operandType = checker->checkExpression(*node.operand);

        switch (node.op) {
        case dotlin::TokenType::MINUS:
          // Unary minus requires numeric type
          if (operandType->kind == dotlin::TypeKind::INT ||
              operandType->kind == dotlin::TypeKind::DOUBLE) {
            result = operandType;
          } else {
            std::cout << "Type error: unary minus requires numeric operand"
                      << std::endl;
            result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
          }
          break;
        case dotlin::TokenType::NOT:
          // Logical NOT requires boolean type
          if (operandType->kind == dotlin::TypeKind::BOOL) {
            result = std::make_shared<dotlin::Type>(dotlin::TypeKind::BOOL);
          } else {
            std::cout << "Type error: logical NOT requires boolean operand"
                      << std::endl;
            result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
          }
          break;
        default:
          result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
          break;
        }
      }

      void visit(dotlin::CallExpr &node) override {
        (void)node; /* For now, assume built-in functions have correct types */
        result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
      }

      void visit(dotlin::MemberAccessExpr &node) override {
        auto objectType = checker->checkExpression(*node.object);
        // For now, return unknown for member access
        result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
      }

      void visit(dotlin::ArrayAccessExpr &node) override {
        auto arrayType = checker->checkExpression(*node.array);
        auto indexType = checker->checkExpression(*node.index);

        // Index must be integer
        if (indexType->kind != dotlin::TypeKind::INT) {
          std::cout << "Type error: array index must be integer" << std::endl;
          result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
        } else {
          // Array access returns the element type
          // For now, return unknown since we don't track element types in the
          // array
          result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
        }
      }

      void visit(dotlin::ArrayLiteralExpr &node) override {
        if (node.elements.empty()) {
          // Empty array - return array of unknown type
          result = std::make_shared<dotlin::Type>(dotlin::TypeKind::ARRAY);
        } else {
          // Check if all elements have the same type
          auto elementType = checker->checkExpression(*node.elements[0]);
          for (size_t i = 1; i < node.elements.size(); ++i) {
            auto currElementType = checker->checkExpression(*node.elements[i]);
            if (!elementType->isCompatibleWith(*currElementType)) {
              std::cout
                  << "Type error: array elements must have compatible types"
                  << std::endl;
              result =
                  std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
              return;
            }
          }
          result = std::make_shared<dotlin::Type>(dotlin::TypeKind::ARRAY,
                                                  elementType);
        }
      }

      void visit(dotlin::LambdaExpr &node) override {
        (void)node;
        result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
      }

      void visit(dotlin::ExpressionStmt &node) override {
        (void)node;
        result = std::make_shared<dotlin::Type>(dotlin::TypeKind::VOID);
      }

      void visit(dotlin::VariableDeclStmt &node) override {
        (void)node;
        result = std::make_shared<dotlin::Type>(dotlin::TypeKind::VOID);
      }

      void visit(dotlin::FunctionDeclStmt &node) override {
        (void)node;
        result = std::make_shared<dotlin::Type>(dotlin::TypeKind::VOID);
      }

      void visit(dotlin::BlockStmt &node) override {
        (void)node;
        result = std::make_shared<dotlin::Type>(dotlin::TypeKind::VOID);
      }

      void visit(dotlin::ReturnStmt &node) override {
        (void)node;
        result = std::make_shared<dotlin::Type>(dotlin::TypeKind::VOID);
      }

      void visit(dotlin::IfStmt &node) override {
        (void)node;
        result = std::make_shared<dotlin::Type>(dotlin::TypeKind::VOID);
      }

      void visit(dotlin::WhileStmt &node) override {
        (void)node;
        result = std::make_shared<dotlin::Type>(dotlin::TypeKind::VOID);
      }

      void visit(dotlin::ForStmt &node) override {
        (void)node;
        result = std::make_shared<dotlin::Type>(dotlin::TypeKind::VOID);
      }

      void visit(dotlin::WhenStmt &node) override {
        (void)node;
        result = std::make_shared<dotlin::Type>(dotlin::TypeKind::VOID);
      }

      void visit(dotlin::TryStmt &node) override {
        (void)node;
        result = std::make_shared<dotlin::Type>(dotlin::TypeKind::VOID);
      }

      void visit(dotlin::ConstructorDeclStmt &node) override {
        (void)node;
        result = std::make_shared<dotlin::Type>(dotlin::TypeKind::VOID);
      }

      void visit(dotlin::ClassDeclStmt &node) override {
        (void)node;
        result = std::make_shared<dotlin::Type>(dotlin::TypeKind::VOID);
      }
    };

    TypeCheckVisitor visitor(this);
    expr.accept(visitor);
    return visitor.result;
  }

  std::shared_ptr<dotlin::Type> checkStatement(dotlin::Statement &stmt) {
    (void)stmt; // Suppress unused parameter warning
    // Use visitor pattern to check statement types
    struct TypeCheckVisitor : public dotlin::AstVisitor {
      std::shared_ptr<dotlin::Type> result;
      TypeChecker *checker;

      TypeCheckVisitor(TypeChecker *chk) : checker(chk) {}

      void visit(dotlin::ExpressionStmt &node) override {
        (void)node;
        result = std::make_shared<dotlin::Type>(dotlin::TypeKind::VOID);
      }

      void visit(dotlin::VariableDeclStmt &node) override {
        (void)node;
        result = std::make_shared<dotlin::Type>(dotlin::TypeKind::VOID);
      }

      void visit(dotlin::FunctionDeclStmt &node) override {
        (void)node;
        result = std::make_shared<dotlin::Type>(dotlin::TypeKind::VOID);
      }

      void visit(dotlin::BlockStmt &node) override {
        (void)node;
        result = std::make_shared<dotlin::Type>(dotlin::TypeKind::VOID);
      }

      void visit(dotlin::LiteralExpr &node) override {
        (void)node;
        result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
      }

      void visit(dotlin::IdentifierExpr &node) override {
        (void)node;
        result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
      }

      void visit(dotlin::BinaryExpr &node) override {
        (void)node;
        result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
      }

      void visit(dotlin::UnaryExpr &node) override {
        (void)node;
        result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
      }

      void visit(dotlin::CallExpr &node) override {
        (void)node;
        result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
      }

      void visit(dotlin::MemberAccessExpr &node) override {
        (void)node;
        result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
      }

      void visit(dotlin::ArrayAccessExpr &node) override {
        (void)node;
        result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
      }

      void visit(dotlin::ArrayLiteralExpr &node) override {
        (void)node;
        result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
      }

      void visit(dotlin::LambdaExpr &node) override {
        (void)node;
        result = std::make_shared<dotlin::Type>(dotlin::TypeKind::UNKNOWN);
      }

      void visit(dotlin::ReturnStmt &node) override {
        (void)node;
        result = std::make_shared<dotlin::Type>(dotlin::TypeKind::VOID);
      }

      void visit(dotlin::IfStmt &node) override {
        (void)node;
        result = std::make_shared<dotlin::Type>(dotlin::TypeKind::VOID);
      }

      void visit(dotlin::WhileStmt &node) override {
        (void)node;
        result = std::make_shared<dotlin::Type>(dotlin::TypeKind::VOID);
      }

      void visit(dotlin::ForStmt &node) override {
        (void)node;
        result = std::make_shared<dotlin::Type>(dotlin::TypeKind::VOID);
      }

      void visit(dotlin::WhenStmt &node) override {
        (void)node;
        result = std::make_shared<dotlin::Type>(dotlin::TypeKind::VOID);
      }

      void visit(dotlin::TryStmt &node) override {
        (void)node;
        result = std::make_shared<dotlin::Type>(dotlin::TypeKind::VOID);
      }

      void visit(dotlin::ConstructorDeclStmt &node) override {
        (void)node;
        result = std::make_shared<dotlin::Type>(dotlin::TypeKind::VOID);
      }

      void visit(dotlin::ClassDeclStmt &node) override {
        (void)node;
        result = std::make_shared<dotlin::Type>(dotlin::TypeKind::VOID);
      }
    };

    TypeCheckVisitor visitor(this);
    stmt.accept(visitor);
    return visitor.result;
  }
};

Value Interpreter::executeBlock(const std::vector<Statement::Ptr> &statements,
                                std::shared_ptr<Environment> parentEnv) {
  // Create new environment that inherits from parent
  auto blockEnv =
      std::make_shared<Environment>(parentEnv, true); // Mark as block scope
  auto previousEnv = environment;
  environment = blockEnv;

  Value result;
  for (const auto &stmt : statements) {
    execute(*stmt);
  }

  environment = previousEnv; // Restore previous environment
  return result;
}

std::shared_ptr<FunctionDef>
Interpreter::findBestFunctionOverload(const std::string &name,
                                      const std::vector<Value> &args) {
  auto it = functionDefinitions.find(name);
  if (it == functionDefinitions.end()) {
    return nullptr;
  }

  // Find function with matching parameter count
  for (const auto &funcCandidate : it->second) {
    if (funcCandidate->parameters.size() == args.size()) {
      // For now, just pick the first match with correct parameter count
      // In a more advanced implementation, we would check parameter types too
      return funcCandidate;
    }
  }

  return nullptr;
}

// Perform type inference on a program
void Interpreter::performTypeInference(Program &program) {
  // Create a type checker instance
  TypeChecker typeChecker(environment);

  // Perform type inference for each statement in the program
  for (auto &stmt : program.statements) {
    performTypeInferenceOnStatement(*stmt, typeChecker);
  }
}

// Perform type inference on a statement
void Interpreter::performTypeInferenceOnStatement(Statement &stmt,
                                                  TypeChecker &typeChecker) {
  // For variable declarations, infer the type from the initializer if no
  // explicit type annotation is provided
  if (auto *varDecl = dynamic_cast<VariableDeclStmt *>(&stmt)) {
    if (!varDecl->typeAnnotation.has_value() &&
        varDecl->initializer.has_value()) {
      // Infer the type from the initializer expression
      auto inferredType =
          typeChecker.checkExpression(*(varDecl->initializer.value()));

      // Store the inferred type in the variable declaration
      varDecl->typeAnnotation = inferredType;

      std::cout << "Type inferred for variable '" << varDecl->name
                << "': " << typeToString(inferredType) << std::endl;
    }
  } else if (auto *blockStmt = dynamic_cast<BlockStmt *>(&stmt)) {
    // Recursively perform type inference on statements in the block
    for (auto &innerStmt : blockStmt->statements) {
      performTypeInferenceOnStatement(*innerStmt, typeChecker);
    }
  } else if (auto *ifStmt = dynamic_cast<IfStmt *>(&stmt)) {
    // Perform type inference on the condition and branches
    if (ifStmt->condition) {
      performTypeInferenceOnExpression(*ifStmt->condition, typeChecker);
    }
    if (ifStmt->thenBranch) {
      performTypeInferenceOnStatement(*ifStmt->thenBranch, typeChecker);
    }
    if (ifStmt->elseBranch.has_value() && ifStmt->elseBranch.value()) {
      performTypeInferenceOnStatement(*(ifStmt->elseBranch.value()),
                                      typeChecker);
    }
  } else if (auto *whileStmt = dynamic_cast<WhileStmt *>(&stmt)) {
    // Perform type inference on the condition and body
    if (whileStmt->condition) {
      performTypeInferenceOnExpression(*whileStmt->condition, typeChecker);
    }
    if (whileStmt->body) {
      performTypeInferenceOnStatement(*whileStmt->body, typeChecker);
    }
  } else if (auto *forStmt = dynamic_cast<ForStmt *>(&stmt)) {
    // Perform type inference on the iterable and body
    if (forStmt->iterable) {
      performTypeInferenceOnExpression(*forStmt->iterable, typeChecker);
    }
    if (forStmt->body) {
      performTypeInferenceOnStatement(*forStmt->body, typeChecker);
    }
  }
  // Add more statement types as needed
}

// Perform type inference on an expression
void Interpreter::performTypeInferenceOnExpression(Expression &expr,
                                                   TypeChecker &typeChecker) {
  // Visit child expressions recursively
  if (auto *binaryExpr = dynamic_cast<BinaryExpr *>(&expr)) {
    if (binaryExpr->left) {
      performTypeInferenceOnExpression(*binaryExpr->left, typeChecker);
    }
    if (binaryExpr->right) {
      performTypeInferenceOnExpression(*binaryExpr->right, typeChecker);
    }
  } else if (auto *unaryExpr = dynamic_cast<UnaryExpr *>(&expr)) {
    if (unaryExpr->operand) {
      performTypeInferenceOnExpression(*unaryExpr->operand, typeChecker);
    }
  } else if (auto *callExpr = dynamic_cast<CallExpr *>(&expr)) {
    for (auto &arg : callExpr->arguments) {
      if (arg) {
        performTypeInferenceOnExpression(*arg, typeChecker);
      }
    }
  } else if (auto *memberAccess = dynamic_cast<MemberAccessExpr *>(&expr)) {
    if (memberAccess->object) {
      performTypeInferenceOnExpression(*memberAccess->object, typeChecker);
    }
  } else if (auto *arrayAccess = dynamic_cast<ArrayAccessExpr *>(&expr)) {
    if (arrayAccess->array) {
      performTypeInferenceOnExpression(*arrayAccess->array, typeChecker);
    }
    if (arrayAccess->index) {
      performTypeInferenceOnExpression(*arrayAccess->index, typeChecker);
    }
  } else if (auto *arrayLiteral = dynamic_cast<ArrayLiteralExpr *>(&expr)) {
    for (auto &element : arrayLiteral->elements) {
      if (element) {
        performTypeInferenceOnExpression(*element, typeChecker);
      }
    }
  }
  // For identifier expressions, we can get the type from the environment
  // but don't need to recurse further
}

} // namespace dotlin
