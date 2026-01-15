#include "dotlin/interpreter.h"
// #include <iostream>
// #include <stdexcept>

namespace dotlin
{

  std::string valueToString(const Value &value);
  std::string typeToString(const std::shared_ptr<Type> &type);

  std::string Interpreter::valueToString(const Value &value)
  {
    return dotlin::valueToString(value);
  }

  std::string Interpreter::typeToString(const std::shared_ptr<Type> &type)
  {
    return dotlin::typeToString(type);
  }

  void Environment::define(const std::string &name, Value value)
  {
    values[name] = value;
    invalidateCache();
  }

  Value Environment::get(const std::string &name)
  {
    // First check the cache if it's valid
    if (cache_valid)
    {
      auto cache_it = lookup_cache.find(name);
      if (cache_it != lookup_cache.end())
      {
        auto &pair = cache_it->second;
        auto &env_ptr = pair.first;
        auto &depth = pair.second;
        auto value_it = env_ptr->values.find(name);
        if (value_it != env_ptr->values.end())
        {
          return value_it->second;
        }
      }
    }

    // If not in cache or cache invalid, do traditional lookup
    auto it = values.find(name);
    if (it != values.end())
    {
      return it->second;
    }

    if (enclosing)
    {
      return enclosing->get(name);
    }

    throw std::runtime_error("Undefined variable: " + name);
  }

  void Environment::assign(const std::string &name, Value value)
  {
    // First check this environment
    auto it = values.find(name);
    if (it != values.end())
    {
      values[name] = value;
      invalidateCache();
      return;
    }

    // If not found here, try enclosing environment
    if (enclosing)
    {
      enclosing->assign(name, value);
      return;
    }

    throw std::runtime_error("Cannot assign to undefined variable: " + name);
  }

  void Environment::invalidateCache()
  {
    cache_valid = false;

    // Invalidate caches in enclosing environments too
    if (enclosing)
    {
      enclosing->invalidateCache();
    }
  }

  void Environment::rebuildCache()
  {
    if (cache_valid)
      return;

    lookup_cache.clear();

    // Collect all variables from this environment
    for (const auto &pair : values)
    {
      lookup_cache[pair.first] = std::make_pair(shared_from_this(), 0);
    }

    // Collect variables from enclosing environments
    std::shared_ptr<Environment> current = enclosing;
    size_t depth = 1;
    while (current)
    {
      for (const auto &pair : current->values)
      {
        // Only add to cache if not already shadowed
        if (lookup_cache.find(pair.first) == lookup_cache.end())
        {
          lookup_cache[pair.first] = std::make_pair(current, depth);
        }
      }
      current = current->enclosing;
      depth++;
    }

    cache_valid = true;
  }

} // namespace dotlin
