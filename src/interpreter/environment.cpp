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

} // namespace dotlin
