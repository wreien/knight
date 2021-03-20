#ifndef KNIGHT_ENV_HPP_INCLUDED
#define KNIGHT_ENV_HPP_INCLUDED

#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "eval.hpp"
#include "value.hpp"

namespace kn::eval {

  class Environment {
  private:
    Environment();

  public:
    static Environment& get();

    void push_frame(std::size_t retaddr, Label result, std::size_t num_temps);
    std::pair<std::size_t, Label> pop_frame();

    Label get_variable(const std::string& name);
    Label get_variable(std::string&& name);

    Label get_literal(String s);
    Label get_literal(Boolean b) const noexcept;
    Label get_literal(Null) const noexcept;

    const std::string& nameof(Label v) const;

    bool has_value(Label v) const;
    const Value& value(Label v) const;
    const Value& assign(Label v, Value x);

#ifndef NDEBUG
    void dump_vars() const;
#endif

  private:
    std::unordered_map<std::string, std::size_t> id_map;
    std::vector<std::optional<Value>> values;
    std::vector<std::string> names;

    std::unordered_map<std::string, std::size_t> stringlit_map;
    std::vector<Value> literals;

    std::vector<std::optional<Value>> temporaries;

    struct StackFrame {
      StackFrame(std::size_t retaddr, Label result, std::size_t num_temps)
        : retaddr(retaddr), result(result), num_temps(num_temps)
      {}

      std::size_t retaddr;
      Label result;
      std::size_t num_temps;
    };
    std::vector<StackFrame> stack;

    auto temps() {
      return temporaries.data() + temporaries.size() - stack.back().num_temps; }
    auto temps() const {
      return temporaries.data() + temporaries.size() - stack.back().num_temps; }
  };

}

#endif // KNIGHT_ENV_HPP_INCLUDED
