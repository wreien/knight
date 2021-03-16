#include "value.hpp"
#include "error.hpp"

#include <charconv>

namespace {
  kn::eval::Number string_to_number(const std::string& s) {
    // not in spec, but tests require "+34" to be parsed as 34
    auto i = s.find_first_not_of("\t\n\r +");

    // explicitly do no error checking
    kn::eval::Number result = 0;
    std::from_chars(s.data() + i, s.data() + s.size(), result);
    return result;
  }
}

namespace kn::eval {

  Expression::~Expression() = default;

  Boolean Value::to_bool() const {
    if (auto x = std::get_if<Boolean>(this)) return *x;
    if (auto x = std::get_if<Number>(this)) return *x != 0;
    if (auto x = std::get_if<String>(this)) return not x->empty();
    return false;  // null
  }

  Number Value::to_number() const {
    if (auto x = std::get_if<Boolean>(this)) return *x ? 1 : 0;
    if (auto x = std::get_if<Number>(this)) return *x;
    if (auto x = std::get_if<String>(this)) return string_to_number(*x);
    return 0;  // null
  }

  String Value::to_string() const& {
    if (auto x = std::get_if<Boolean>(this)) return *x ? "true" : "false";
    if (auto x = std::get_if<Number>(this)) return std::to_string(*x);
    if (auto x = std::get_if<String>(this)) return *x;
    return "null";  // null
  }

  // optimise common case of copying from expiring value
  String Value::to_string() && {
    if (auto x = std::get_if<Boolean>(this)) return *x ? "true" : "false";
    if (auto x = std::get_if<Number>(this)) return std::to_string(*x);
    if (auto x = std::get_if<String>(this)) return std::move(*x);
    return "null";  // null
  }

  Block Value::to_block() const {
    if (auto x = std::get_if<Block>(this)) return *x;
    throw kn::Error("error: not a block");
  }
}
