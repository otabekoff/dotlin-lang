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

  /* std::cout << "[DEBUG] Environment(" << this << ")::get failed for '" <<
     name
            << "'" << std::endl; */
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

void Environment::defineAt(int index, Value value) {
  if (static_cast<size_t>(index) >= indexedValues.size()) {
    indexedValues.resize(static_cast<size_t>(index) + 1);
  }
  indexedValues[static_cast<size_t>(index)] = value;
}

Value Environment::getAt(int distance, int index) {
  auto env = ancestor(distance);
  if (static_cast<size_t>(index) < env->indexedValues.size()) {
    return env->indexedValues[static_cast<size_t>(index)];
  }
  // Fallback or error? Resolver should ensure index is valid.
  return Value();
}

void Environment::assignAt(int distance, int index, Value value) {
  auto env = ancestor(distance);
  if (static_cast<size_t>(index) >= env->indexedValues.size()) {
    env->indexedValues.resize(static_cast<size_t>(index) + 1);
  }
  env->indexedValues[static_cast<size_t>(index)] = value;
}

} // namespace dotlin
