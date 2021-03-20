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

  Value::Value(const Value& other)
    : type(other.type)
  {
    switch (type) {
    case Type::Boolean: boolean = other.boolean; break;
    case Type::Number: number = other.number; break;
    case Type::Block: block = other.block; break;
    case Type::Null: break;
    case Type::String: new (&string) String(other.string);
    }
  }

  Value::Value(Value&& other) noexcept
    : type(other.type)
  {
    switch (type) {
    case Type::Boolean: boolean = other.boolean; break;
    case Type::Number: number = other.number; break;
    case Type::Block: block = other.block; break;
    case Type::Null: break;
    case Type::String: new (&string) String(std::move(other.string));
    }
  }

  Value& Value::operator=(const Value& other) {
    if (this == &other)
      return *this;
    if (type == Type::String)
      string.~String();
    type = other.type;
    switch (type) {
    case Type::Boolean: boolean = other.boolean; break;
    case Type::Number: number = other.number; break;
    case Type::Block: block = other.block; break;
    case Type::Null: break;
    case Type::String: new (&string) String(other.string);
    }
    return *this;
  }

  Value& Value::operator=(Value&& other) noexcept {
    if (this == &other)
      return *this;
    if (type == Type::String)
      string.~String();
    type = other.type;  // don't exchange
    switch (type) {
    case Type::Boolean: boolean = other.boolean; break;
    case Type::Number: number = other.number; break;
    case Type::Block: block = other.block; break;
    case Type::Null: break;
    case Type::String: new (&string) String(std::move(other.string));
    }
    return *this;
  }

  Value::~Value() {
    if (type == Type::String)
      string.~String();
  }

  Boolean Value::to_bool() const {
    if (type == Type::Boolean) return boolean;
    if (type == Type::Number) return number != 0;
    if (type == Type::String) return not string.as_str().empty();
    return false;  // null
  }

  Number Value::to_number() const {
    if (type == Type::Boolean) return boolean ? 1 : 0;
    if (type == Type::Number) return number;
    if (type == Type::String) return string_to_number(string);
    return 0;  // null
  }

  String Value::to_string() const& {
    if (type == Type::Boolean) return boolean ? true_str : false_str;
    if (type == Type::Number) return String(std::to_string(number));
    if (type == Type::String) return string;
    return null_str;  // null
  }

  // optimise common case of copying from expiring value
  String Value::to_string() && {
    if (type == Type::Boolean) return boolean ? true_str : false_str;
    if (type == Type::Number) return String(std::to_string(number));
    if (type == Type::String) return std::move(string);
    return null_str;  // null
  }

  Block Value::to_block() const {
    if (type != Type::Block)
      throw kn::Error("error: not a block");
    return Block{ block };
  }
}
