#ifndef KNIGHT_VALUE_HPP_INCLUDED
#define KNIGHT_VALUE_HPP_INCLUDED

#include <memory>
#include <ostream>
#include <string>
#include <variant>

namespace kn::eval {

  struct Value;

  // basic types
  struct Null {
    friend bool operator==(Null, Null) noexcept { return true; }
  };

  // TODO: one day go through and make these operators `explicit`

  struct Boolean {
    Boolean(bool value) : value(value) {}
    operator bool() const { return value; }
    friend bool operator==(Boolean a, Boolean b) noexcept { return a.value == b.value; }
    bool value;
  };

  struct Number {
    using type = int;
    Number(type value) : value(value) {}
    operator type() const { return value; }
    friend bool operator==(Number a, Number b) noexcept { return a.value == b.value; }
    type value;
  };

  struct String {
    explicit String(std::string&& value)
      : value(std::make_shared<std::string>(std::move(value)))
    {}
    explicit String(std::string_view value)
      : value(std::make_shared<std::string>(value))
    {}

    explicit operator const std::string&() const noexcept { return *value; }
    explicit operator std::string_view() const noexcept { return *value; }

    const std::string& as_str() const noexcept { return *value; }
    std::string_view as_str_view() const noexcept { return *value; }

    // some functions that this is used for
    auto size() const noexcept { return value->size(); }
    String substr(std::size_t pos, std::size_t len) const;
    String replace(std::size_t pos, std::size_t len, const String& other) const;
    void output(std::ostream& os) const;

    friend String operator+(const String& lhs, const String& rhs);
    friend String operator*(const String& lhs, Number rhs);

    friend bool operator==(const String& a, const String& b) noexcept {
      return a.value == b.value or *a.value == *b.value;
    }
    friend std::ostream& operator<<(std::ostream& os, const String& s) {
      return os << *s.value;
    }

    std::shared_ptr<const std::string> value;
  };

  struct Block {
    friend bool operator==(Block, Block) noexcept { return false; }
    std::size_t address;
  };

  struct Value : std::variant<Null, Boolean, Number, String, Block> {
    using variant::variant;

    Value(Number::type x) : variant(Number(x)) {}
    Value(bool x) : variant(Boolean(x)) {}

    bool is_null() const noexcept { return std::holds_alternative<Null>(*this); }
    bool is_bool() const noexcept { return std::holds_alternative<Boolean>(*this); }
    bool is_number() const noexcept { return std::holds_alternative<Number>(*this); }
    bool is_string() const noexcept { return std::holds_alternative<String>(*this); }

    Boolean to_bool() const;
    Number to_number() const;
    String to_string() const&;
    String to_string() &&;
    Block to_block() const;

    friend std::ostream& operator<<(std::ostream& os, const Value& value) {
      if (std::holds_alternative<Null>(value))
        os << "Null()";
      else if (auto x = std::get_if<Boolean>(&value))
        os << "Boolean(" << (*x ? "true" : "false") << ")";
      else if (auto x = std::get_if<Number>(&value))
        os << "Number(" << *x << ")";
      else if (auto x = std::get_if<String>(&value))
        os << "String(" << *x << ")";
      else if (auto x = std::get_if<Block>(&value))
        os << "Function(" << x->address << ")";
      else
        os << "???";
      return os;
    }
  };

}

#endif // KNIGHT_VALUE_HPP_INCLUDED
