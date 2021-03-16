#ifndef KNIGHT_ENV_HPP_INCLUDED
#define KNIGHT_ENV_HPP_INCLUDED

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "parser.hpp"

namespace kn::eval {

  struct Variable {
    friend bool operator==(Variable a, Variable b) noexcept { return a.id == b.id; }
    std::size_t id;
  };

  class Environment {
  public:
    static Environment& get();

    Variable get_variable(const std::string& name);

    std::string nameof(const Variable& v) const {
      return names[v.id];
    }

    const Value& value(const Variable& v) const;
    const Value& assign(const Variable& v, Value x);

  private:
    Environment();

    std::unordered_map<std::string, std::size_t> id_map;
    std::vector<std::optional<Value>> values;
    std::vector<std::string> names;
  };

  struct IdentExpr : kn::Expression {
    IdentExpr(const std::string& name);

    Value evaluate() const override {
      return Environment::get().value(data);
    }
    void dump() const override;

    Variable data;
  };

}

#endif // KNIGHT_ENV_HPP_INCLUDED
