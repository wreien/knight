#include "env.hpp"

#include <cassert>
#include <iostream>

#include "error.hpp"

namespace kn::eval {

  Environment::Environment()
    : id_map()
    , values()
    , names()
    , stringlit_map()
    , literals{ { Null{} }, { true }, { false } }
    , temporaries()
    , stack()
  {}

  Environment& Environment::get() {
    static Environment env;
    return env;
  }

  void Environment::push_frame(
    std::size_t retaddr, Label result, std::size_t num_temps)
  {
    stack.emplace_back(retaddr, result, num_temps);
    temporaries.resize(temporaries.size() + num_temps);
  }

  std::pair<std::size_t, Label> Environment::pop_frame() {
    auto res = std::pair{ stack.back().retaddr, stack.back().result };
    temporaries.resize(temporaries.size() - stack.back().num_temps);
    stack.pop_back();
    return res;
  }

  Label Environment::get_variable(const std::string& name) {
    auto [it, inserted] = id_map.try_emplace(name, values.size());
    if (inserted) {
      names.push_back(name);
      values.emplace_back();
    }
    return { LabelCat::Variable, it->second };
  }
  Label Environment::get_variable(std::string&& name) {
    auto [it, inserted] = id_map.try_emplace(name, values.size());
    if (inserted) {
      names.push_back(std::move(name));
      values.emplace_back();
    }
    return { LabelCat::Variable, it->second };
  }

  Label Environment::get_literal(std::string s) {
    auto [it, inserted] = stringlit_map.try_emplace(s, literals.size());
    if (inserted) {
      literals.emplace_back(String(std::move(s)));
    }
    return { LabelCat::Literal, it->second };
  }

  Label Environment::get_literal(Boolean b) const noexcept {
    return { LabelCat::Literal, b ? 1u : 2u };
  }

  Label Environment::get_literal(Null) const noexcept {
    return { LabelCat::Literal, 0 };
  }

  const std::string& Environment::nameof(Label v) const {
    assert(v.cat() == LabelCat::Variable);
    return names[v.id()];
  }

  bool Environment::has_value(Label v) const {
    assert(v.needs_eval());
    return v.cat() == LabelCat::Literal
      or (v.cat() == LabelCat::Variable and values[v.id()])
      or (v.cat() == LabelCat::Temporary and temps()[v.id()]);
  }

  const Value& Environment::value(Label v) const {
    assert(v.needs_eval());
    if (v.cat() == LabelCat::Variable) {
      if (not values[v.id()]) {
        throw kn::Error("error: evaluating undefined variable " + names[v.id()]);
      }
      return *values[v.id()];
    } else if (v.cat() == LabelCat::Literal) {
      return literals[v.id()];
    } else if (v.cat() == LabelCat::Temporary) {
      assert(temps()[v.id()].has_value());
      return *temps()[v.id()];
    }
#ifdef __GNUC__
    __builtin_unreachable();
#elif _WIN32
    __assume(0);
#endif
  }

#ifndef NDEBUG
  void Environment::dump_vars() const {
    for (std::size_t i = 0; i < values.size(); ++i) {
      std::cout << "[v:" << i << "] " << names[i] << " => ";
      if (values[i])
        std::cout << *values[i];
      else
        std::cout << "#";
      std::cout << '\n';
    }
  }
#endif

  const Value& Environment::assign(Label v, Value x) {
    // must be a variable or temporary to write to it
    assert(v.cat() == LabelCat::Variable or v.cat() == LabelCat::Temporary);

    if (v.cat() == LabelCat::Variable) {
      return values[v.id()].emplace(std::move(x));
    } else {
      return temps()[v.id()].emplace(std::move(x));
    }
  }

}
