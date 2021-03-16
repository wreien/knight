#ifndef KNIGHT_VALUE_HPP_INCLUDED
#define KNIGHT_VALUE_HPP_INCLUDED

#include <charconv>
#include <ostream>
#include <string>
#include <variant>

namespace kn::eval {

  // basic types
  struct Null {
    friend bool operator==(Null, Null) noexcept { return true; }
  };

  using Boolean = bool;
  using Number = int;
  using String = std::string;

  struct Value : std::variant<Null, Boolean, Number, String> {
    using variant::variant;

    bool is_null() const noexcept { return std::holds_alternative<Null>(*this); }
    bool is_bool() const noexcept { return std::holds_alternative<Boolean>(*this); }
    bool is_number() const noexcept { return std::holds_alternative<Number>(*this); }
    bool is_string() const noexcept { return std::holds_alternative<String>(*this); }

    Boolean to_bool() const {
      if (auto x = std::get_if<Boolean>(this)) return *x;
      if (auto x = std::get_if<Number>(this)) return *x != 0;
      if (auto x = std::get_if<String>(this)) return x->empty();
      return false;  // null
    }

    Number to_number() const {
      if (auto x = std::get_if<Boolean>(this)) return *x ? 1 : 0;
      if (auto x = std::get_if<Number>(this)) return *x;
      if (auto x = std::get_if<String>(this)) return string_to_number(*x);
      return 0;  // null
    }

    String to_string() const& {
      if (auto x = std::get_if<Boolean>(this)) return *x ? "true" : "false";
      if (auto x = std::get_if<Number>(this)) return std::to_string(*x);
      if (auto x = std::get_if<String>(this)) return *x;
      return "null";  // null
    }

    // optimise common case of copying from expiring value
    String to_string() && {
      if (auto x = std::get_if<Boolean>(this)) return *x ? "true" : "false";
      if (auto x = std::get_if<Number>(this)) return std::to_string(*x);
      if (auto x = std::get_if<String>(this)) return std::move(*x);
      return "null";  // null
    }

    friend std::ostream& operator<<(std::ostream& os, const Value& value) {
      if (std::holds_alternative<Null>(value))
        os << "Null()";
      else if (auto x = std::get_if<Boolean>(&value))
        os << "Boolean(" << (*x ? "true" : "false") << ")";
      else if (auto x = std::get_if<Number>(&value))
        os << "Number(" << *x << ")";
      else if (auto x = std::get_if<String>(&value))
        os << "String(" << *x << ")";
      else
        os << "???";
      return os;
    }

  private:
    static Number string_to_number(const std::string& s) {
      auto i = s.find_first_not_of("\t\n\r ");

      // explicitly do no error checking
      Number result = 0;
      std::from_chars(s.data() + i, s.data() + s.size(), result);
      return result;
    }
  };

}

#endif // KNIGHT_VALUE_HPP_INCLUDED
