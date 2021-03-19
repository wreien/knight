#ifndef KNIGHT_ENV_HPP_INCLUDED
#define KNIGHT_ENV_HPP_INCLUDED

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "eval.hpp"
#include "value.hpp"

namespace kn::eval {

  class Environment {
  public:
    static Environment& get();

    Label get_variable(const std::string& name);
    Label get_temp();

    Label get_literal(String s);
    Label get_literal(Boolean b) const noexcept;
    Label get_literal(Null) const noexcept;

    Label get_jump();

    const std::string& nameof(const Label& v) const;

    bool has_value(const Label& v) const;
    const Value& value(const Label& v) const;
    const Value& assign(const Label& v, Value x);

#ifndef NDEBUG
    void dump_vars() const;
#endif

  private:
    Environment();

    std::unordered_map<std::string, std::size_t> id_map;
    std::unordered_map<String, std::size_t> stringlit_map;
    std::vector<std::optional<Value>> values;
    std::vector<std::string> names;

    std::size_t jumptarget_id;
  };

}

#endif // KNIGHT_ENV_HPP_INCLUDED
