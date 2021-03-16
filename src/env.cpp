#include "env.hpp"
#include "error.hpp"
#include <iostream>

namespace kn::eval {

  Environment::Environment() = default;

  Environment& Environment::get() {
    static Environment env;
    return env;
  }

  Variable Environment::get_variable(const std::string& name) {
    auto [it, inserted] = id_map.try_emplace(name, values.size());
    if (inserted) {
      names.push_back(name);
      values.emplace_back();
    }
    return { it->second };
  }

  const Value& Environment::value(const Variable& v) const {
    if (not values[v.id]) {
      throw kn::Error("error: evaluating undefined variable " + names[v.id]);
    }
    return *values[v.id];
  }

  const Value& Environment::assign(const Variable& v, Value x) {
    return values[v.id].emplace(std::move(x));
  }

  IdentExpr::IdentExpr(const std::string& name)
    : data(Environment::get().get_variable(name))
  {}

  void IdentExpr::dump() const {
    std::cout << "Identifier(" << Environment::get().nameof(data) << ")";
  }

}
