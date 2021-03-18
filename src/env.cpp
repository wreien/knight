#include "env.hpp"

#include <cassert>
#include <iostream>

#include "error.hpp"

namespace kn::eval {

  Environment::Environment()
    : id_map()
    , stringlit_map()
    , values{ { Null{} }, { true }, { false } }
    , names{ "NULL", "TRUE", "FALSE" }
    , jumptarget_id{ 0 }
  {}

  Environment& Environment::get() {
    static Environment env;
    return env;
  }

  Label Environment::get_variable(const std::string& name) {
    auto [it, inserted] = id_map.try_emplace(name, values.size());
    if (inserted) {
      names.push_back(name);
      values.emplace_back();
    }
    return { LabelCat::Variable, it->second };
  }

  Label Environment::get_temp() {
    names.push_back("v:" + std::to_string(names.size()));
    values.emplace_back();
    return { LabelCat::Variable, values.size() - 1 };
  }

  Label Environment::get_literal(String s) {
    auto [it, inserted] = stringlit_map.try_emplace(s, values.size());
    if (inserted) {
      names.emplace_back("s:" + std::to_string(names.size()));
      values.emplace_back(std::move(s));
    }
    return { LabelCat::Variable, it->second };
  }

  Label Environment::get_literal(Boolean b) const noexcept {
    return { LabelCat::Variable, b ? 1u : 2u };
  }

  Label Environment::get_literal(Null) const noexcept {
    return { LabelCat::Variable, 0 };
  }

  Label Environment::get_jump() {
    return { LabelCat::JumpTarget, jumptarget_id++ };
  }

  const std::string& Environment::nameof(const Label& v) const {
    assert(v.cat() == LabelCat::Variable);
    return names[v.id()];
  }

  const Value& Environment::value(const Label& v) const {
    assert(v.cat() == LabelCat::Variable);
    if (not values[v.id()]) {
      throw kn::Error("error: evaluating undefined variable " + names[v.id()]);
    }
    return *values[v.id()];
  }

  const Value& Environment::assign(const Label& v, Value x) {
    assert(v.cat() == LabelCat::Variable);
    assert(v.id() >= 3);
    return values[v.id()].emplace(std::move(x));
  }

}
