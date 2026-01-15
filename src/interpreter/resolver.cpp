#include "dotlin/visitors.h"
// #include <iostream>

namespace dotlin {

void ResolverVisitor::resolve(const std::vector<Statement::Ptr> &statements) {
  for (const auto &stmt : statements) {
    resolve(stmt);
  }
}

void ResolverVisitor::resolve(const Statement::Ptr &stmt) {
  if (stmt) {
    stmt->accept(*this);
  }
}

void ResolverVisitor::resolve(const Expression::Ptr &expr) {
  if (expr) {
    expr->accept(*this);
  }
}

void ResolverVisitor::beginScope() {
  scopes.push_back(std::unordered_map<std::string, ScopeEntry>());
  scopeIndexCounters.push_back(0);
}

void ResolverVisitor::endScope() {
  scopes.pop_back();
  scopeIndexCounters.pop_back();
}

void ResolverVisitor::declare(const std::string &name) {
  if (scopes.empty()) {
    return;
  }
  auto &scope = scopes.back();
  if (scope.find(name) != scope.end()) {
    // Variable already declared
    return;
  }
  int index = scopeIndexCounters.back()++;
  scope[name] = {false, index};
}

void ResolverVisitor::define(const std::string &name) {
  if (scopes.empty()) {
    return;
  }
  scopes.back()[name].defined = true;
}

void ResolverVisitor::resolveLocal(Expression &expr, const std::string &name) {
  for (int i = static_cast<int>(scopes.size()) - 1; i >= 0; i--) {
    auto it = scopes[static_cast<size_t>(i)].find(name);
    if (it != scopes[static_cast<size_t>(i)].end()) {
      int distance = static_cast<int>(scopes.size()) - 1 - i;
      int index = it->second.index;
      interpreter->resolve(&expr, distance, index);
      return;
    }
  }
}

void ResolverVisitor::visit(BlockStmt &node) {
  beginScope();
  resolve(node.statements);
  endScope();
}

void ResolverVisitor::visit(VariableDeclStmt &node) {
  declare(node.name);
  if (node.initializer) {
    resolve(node.initializer.value());
  }
  define(node.name);

  // Store the index in the AST node for use during execution if it's a local
  if (!scopes.empty()) {
    node.index = scopes.back()[node.name].index;
  }
}

void ResolverVisitor::visit(FunctionDeclStmt &node) {
  declare(node.name);
  define(node.name);

  FunctionType enclosingFunction = currentFunction;
  currentFunction = FunctionType::FUNCTION;

  beginScope();
  for (const auto &param : node.parameters) {
    declare(param.name);
    define(param.name);
  }
  resolve(node.body);
  endScope();
  currentFunction = enclosingFunction;
}

void ResolverVisitor::visit(ExpressionStmt &node) { resolve(node.expression); }

void ResolverVisitor::visit(IfStmt &node) {
  resolve(node.condition);
  resolve(node.thenBranch);
  if (node.elseBranch) {
    resolve(node.elseBranch.value());
  }
}

void ResolverVisitor::visit(WhileStmt &node) {
  resolve(node.condition);
  resolve(node.body);
}

void ResolverVisitor::visit(ReturnStmt &node) {
  if (currentFunction == FunctionType::NONE) {
    // Error: return from top-level code?
  }
  if (node.value) {
    resolve(node.value);
  }
}

void ResolverVisitor::visit(ClassDeclStmt &node) {
  declare(node.name);
  define(node.name);

  // Resolve superclass
  if (node.superClass) {
    // TODO: Resolve super class identifier? It's a string name here.
    // In Dotlin generic parser, superClass is string. We should try to resolve
    // it if it refers to a var? Actually classes are usually top-level or
    // handled by declare/define. If superclass is an expression, we'd resolve
    // it. But it's just a name string. We treat class names as variable
    // references in Lox. Here, we might need to assume it's resolved or global.
    // For now, let's treat it as a variable access if we can, but
    // IdentifierExpr logic handles lookups. Is there an IdentifierExpr for
    // superclass? No, it's std::string. If we want to resolve it, we'd need to
    // simulate an access.
  }

  // Resolve members
  for (const auto &member : node.members) {
    if (auto func = std::dynamic_pointer_cast<FunctionDeclStmt>(member)) {
      FunctionType declaration = FunctionType::METHOD;
      FunctionType enclosingFunction = currentFunction;
      currentFunction = declaration;

      beginScope();
      declare("this");
      define("this");
      for (const auto &param : func->parameters) {
        declare(param.name);
        define(param.name);
      }
      resolve(func->body);
      endScope();
      currentFunction = enclosingFunction;
    } else if (auto ctor =
                   std::dynamic_pointer_cast<ConstructorDeclStmt>(member)) {
      FunctionType enclosingFunction = currentFunction;
      currentFunction = FunctionType::INITIALIZER;

      beginScope();
      declare("this");
      define("this");
      for (const auto &param : ctor->parameters) {
        declare(param.name);
        define(param.name);
      }
      resolve(ctor->body);
      endScope();
      currentFunction = enclosingFunction;
    } else if (auto var = std::dynamic_pointer_cast<VariableDeclStmt>(member)) {
      // Field: resolve initializer only, don't declare in any local scope
      if (var->initializer) {
        resolve(var->initializer.value());
      }
    }
  }
}

void ResolverVisitor::visit(IdentifierExpr &node) {
  if (!scopes.empty()) {
    auto it = scopes.back().find(node.name);
    if (it != scopes.back().end() && it->second.defined == false) {
      // Error: Cannot read local variable in its own initializer
    }
  }
  resolveLocal(node, node.name);
}

void ResolverVisitor::visit(BinaryExpr &node) {
  resolve(node.left);
  resolve(node.right);
}

void ResolverVisitor::visit(CallExpr &node) {
  resolve(node.callee);
  for (const auto &arg : node.arguments) {
    resolve(arg);
  }
}

void ResolverVisitor::visit(LambdaExpr &node) {
  FunctionType enclosingFunction = currentFunction;
  currentFunction = FunctionType::FUNCTION; // Treat lambda as function

  beginScope();
  for (const auto &param : node.parameters) {
    declare(param.name);
    define(param.name);
  }
  resolve(node.body); // Body is Statement::Ptr (usually BlockStmt or
                      // ExpressionStmt? No, Body is Statement::Ptr)
  endScope();
  currentFunction = enclosingFunction;
}

void ResolverVisitor::visit(ForStmt &node) {
  resolve(node.iterable);
  beginScope();
  declare(node.variable);
  define(node.variable);
  resolve(node.body);
  endScope();
}

void ResolverVisitor::visit(WhenStmt &node) {
  resolve(node.subject);
  for (auto &branch : node.branches) {
    resolve(branch.first);  // condition
    resolve(branch.second); // body statement
  }
  if (node.elseBranch) {
    resolve(node.elseBranch.value());
  }
}

void ResolverVisitor::visit(TryStmt &node) {
  resolve(node.tryBlock);
  beginScope();
  declare(node.exceptionVar);
  define(node.exceptionVar);
  resolve(node.catchBlock);
  endScope();
  if (node.finallyBlock) {
    resolve(node.finallyBlock.value());
  }
}

void ResolverVisitor::visit(ConstructorDeclStmt &node) {
  FunctionType enclosingFunction = currentFunction;
  currentFunction = FunctionType::INITIALIZER;

  beginScope();
  for (const auto &param : node.parameters) {
    declare(param.name);
    define(param.name);
  }
  resolve(node.body);
  endScope();
  currentFunction = enclosingFunction;
}

void ResolverVisitor::visit(ExtensionFunctionDeclStmt &node) {
  declare(node.name);
  define(node.name);

  FunctionType enclosingFunction = currentFunction;
  currentFunction = FunctionType::FUNCTION;
  beginScope();
  // Implicit 'this' for receiver
  declare("this");
  define("this");

  for (const auto &param : node.parameters) {
    declare(param.name);
    define(param.name);
  }
  resolve(node.body);
  endScope();
  currentFunction = enclosingFunction;
}

// Other expressions
void ResolverVisitor::visit(LiteralExpr &node) { (void)node; }
void ResolverVisitor::visit(StringInterpolationExpr &node) {
  for (const auto &part : node.parts) {
    resolve(part);
  }
}
void ResolverVisitor::visit(UnaryExpr &node) { resolve(node.operand); }
void ResolverVisitor::visit(MemberAccessExpr &node) {
  resolve(node.object);
  // property is a name, but it's looked up on the object, not in scope chain
}
void ResolverVisitor::visit(ArrayLiteralExpr &node) {
  for (const auto &elem : node.elements) {
    resolve(elem);
  }
}
void ResolverVisitor::visit(ArrayAccessExpr &node) {
  resolve(node.array);
  resolve(node.index);
}
} // namespace dotlin
