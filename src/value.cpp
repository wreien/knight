#include "value.hpp"
#include "error.hpp"
#include "env.hpp"

#include <charconv>

namespace {
  kn::eval::Number string_to_number(const kn::eval::String& s) {
    // not in spec, but tests require "+34" to be parsed as 34
    auto i = s.as_str().find_first_not_of("\t\n\r +");

    // explicitly do no error checking
    kn::eval::Number::type result = 0;
    std::from_chars(s.value->data() + i, s.value->data() + s.size(), result);
    return result;
  }

  kn::eval::String true_str(std::string("true"));
  kn::eval::String false_str(std::string("false"));
  kn::eval::String null_str(std::string("null"));
}

namespace kn::eval {

  String String::substr(std::size_t pos, std::size_t len) const {
    return String(value->substr(pos, len));
  }

  String String::replace(std::size_t pos, std::size_t len, const String& other) const {
    auto str = as_str();
    return String(str.replace(pos, len, other.as_str()));
  }

  void String::output(std::ostream& os) const {
    auto view = as_str_view();
    if (not view.empty() and view.back() == '\\') {
      view.remove_suffix(1);
      os << view << std::flush;
    } else {
      os << view << '\n' << std::flush;
    }
  }

  String operator+(const String& lhs, const String& rhs) {
    return String(lhs.as_str() + rhs.as_str());
  }

  String operator*(const String& lhs, Number rhs) {
    auto str = std::string{};
    auto num = static_cast<std::size_t>(rhs);
    str.reserve(lhs.size() * num);
    while (num--) str += lhs.as_str();
    return String(std::move(str));
  }

  Boolean Value::to_bool() const {
    if (auto x = std::get_if<Boolean>(this)) return *x;
    if (auto x = std::get_if<Number>(this)) return *x != 0;
    if (auto x = std::get_if<String>(this)) return not x->as_str().empty();
    return false;  // null
  }

  Number Value::to_number() const {
    if (auto x = std::get_if<Boolean>(this)) return *x ? 1 : 0;
    if (auto x = std::get_if<Number>(this)) return *x;
    if (auto x = std::get_if<String>(this)) return string_to_number(*x);
    return 0;  // null
  }

  String Value::to_string() const& {
    if (auto x = std::get_if<Boolean>(this)) return *x ? true_str : false_str;
    if (auto x = std::get_if<Number>(this)) return String(std::to_string(*x));
    if (auto x = std::get_if<String>(this)) return *x;
    return null_str;  // null
  }

  // optimise common case of copying from expiring value
  String Value::to_string() && {
    if (auto x = std::get_if<Boolean>(this)) return *x ? true_str : false_str;
    if (auto x = std::get_if<Number>(this)) return String(std::to_string(*x));
    if (auto x = std::get_if<String>(this)) return std::move(*x);
    return null_str;  // null
  }

  Block Value::to_block() const {
    if (auto x = std::get_if<Block>(this)) return *x;
    throw kn::Error("error: not a block");
  }
}
