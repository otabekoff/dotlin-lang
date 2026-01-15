#include "dotlin/interpreter.h"
#include <stdexcept>

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

  std::cout << "[DEBUG] Environment(" << this << ")::get failed for '" << name
            << "'" << std::endl;
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

std::shared_ptr<Environment> Environment::ancestor(int distance) {
  std::shared_ptr<Environment> environment = shared_from_this();
  for (int i = 0; i < distance; i++) {
    if (environment->enclosing) {
      environment = environment->enclosing;
    } else {
      // Should not happen if resolver is correct
      return nullptr;
    }
  }
  return environment;
}

Value Environment::getAt(int distance, const std::string &name) {
  return ancestor(distance)
      ->values[name]; // or find() to be safe? Resolver helps safety.
  // ancestor(distance)->values.find(name)->second;
}

void Environment::assignAt(int distance, const std::string &name, Value value) {
  ancestor(distance)->values[name] = value;
}

} // namespace dotlin
