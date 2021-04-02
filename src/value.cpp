#include "value.hpp"
#include "error.hpp"
#include "env.hpp"

#include <memory>
#include <charconv>

namespace {
  kn::eval::Number string_to_number(const kn::eval::String& s) {
    // not in spec, but tests require "+34" to be parsed as 34
    auto sv = s.as_str_view();
    auto i = sv.find_first_not_of("\t\n\r +");

    // explicitly do no error checking
    kn::eval::Number::type result = 0;
    std::from_chars(sv.data() + i, sv.data() + sv.size(), result);
    return result;
  }

  kn::eval::String true_str(std::string("true"));
  kn::eval::String false_str(std::string("false"));
  kn::eval::String null_str(std::string("null"));

  constexpr auto value_offset = 2 * sizeof(std::size_t);
  char* alloc_string(std::size_t size) {
    auto p = static_cast<char*>(::operator new(value_offset + size));
    new (p) std::size_t{ size };
    new (p + sizeof(std::size_t)) std::size_t{ 1 };
    return p;
  }
}

namespace kn::eval {

  String::String(std::string_view value)
    : m_data(alloc_string(value.size()))
  {
    auto p = static_cast<char*>(m_data);
    std::uninitialized_copy(value.begin(), value.end(), p + value_offset);
  }

  String String::substr(std::size_t pos, std::size_t len) const {
    return String(as_str_view().substr(pos, len));
  }

  String String::replace(std::size_t pos, std::size_t len, const String& other) const {
    auto p = alloc_string(size() - len + other.size());
    std::uninitialized_copy(
      value(), value() + pos, p + value_offset);
    std::uninitialized_copy(
      other.value(), other.value() + other.size(), p + value_offset + pos);
    std::uninitialized_copy(
      value() + pos + len, value() + size(), p + value_offset + pos + other.size());
    return String(p);
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
    auto p = alloc_string(lhs.size() + rhs.size());
    std::uninitialized_copy(
      lhs.value(), lhs.value() + lhs.size(), p + value_offset);
    std::uninitialized_copy(
      rhs.value(), rhs.value() + rhs.size(), p + value_offset + lhs.size());
    return String(p);
  }

  String operator*(const String& lhs, Number rhs) {
    auto num = static_cast<std::size_t>(rhs);
    auto p = alloc_string(lhs.size() * num);

    auto value = p + 2 * sizeof(std::size_t);
    while (num--) {
      std::uninitialized_copy(lhs.value(), lhs.value() + lhs.size(), value);
      value += lhs.size();
    }

    return String(p);
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
    if (type == Type::String) return string.size() != 0;
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
