#ifndef KNIGHT_VALUE_HPP_INCLUDED
#define KNIGHT_VALUE_HPP_INCLUDED

#include <new>
#include <ostream>
#include <utility>
#include <string>

namespace kn::eval {

  class Value;

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

  // a ref-counted COW string
  class String {
  public:
    explicit String(const std::string& value) : String(std::string_view(value)) {};
    explicit String(std::string_view value);

    ~String() { release(); }

    String(const String& other) : m_data(other.m_data) { ++num_refs(); }
    String(String&& other) noexcept : m_data(std::exchange(other.m_data, nullptr)) {}

    String& operator=(const String& other) {
      if (this != &other) {
        release();
        m_data = other.m_data;
        ++num_refs();
      }
      return *this;
    }
    String& operator=(String&& other) noexcept {
      m_data = std::exchange(other.m_data, nullptr);
      return *this;
    }

    std::string as_str() const {
      return { value(), size() };
    }
    std::string_view as_str_view() const noexcept {
      return { value(), size() };
    }

    // some functions that this is used for
    std::size_t size() const noexcept {
      return *std::launder(reinterpret_cast<std::size_t*>(m_data));
    }

    String substr(std::size_t pos, std::size_t len) const;
    String replace(std::size_t pos, std::size_t len, const String& other) const;
    void output(std::ostream& os) const;

    friend String operator+(const String& lhs, const String& rhs);
    friend String operator*(const String& lhs, Number rhs);

    friend bool operator==(const String& a, const String& b) noexcept {
      return a.m_data == b.m_data or a.as_str_view() == b.as_str_view();
    }
    friend std::ostream& operator<<(std::ostream& os, const String& s) {
      return os << s.as_str_view();
    }

  private:
    explicit String(void* data) : m_data(data) {}
    void* m_data = nullptr;

    std::size_t& num_refs() noexcept {
      return *std::launder(reinterpret_cast<std::size_t*>(
          static_cast<char*>(m_data) + sizeof(std::size_t)));
    }

    const char* value() const noexcept {
      return static_cast<const char*>(m_data) + 2 * sizeof(std::size_t);
    }

    void release() noexcept {
      if (not m_data)
        return;
      if (--num_refs() == 0)
        ::operator delete(m_data);
    }
  };

  struct Block {
    friend bool operator==(Block, Block) noexcept { return false; }
    std::size_t address;
  };

  class Value {
    enum class Type {
      Null, Boolean, Number, String, Block
    };

  public:
    Value() : type(Type::Null) {}
    Value(Null) : type(Type::Null) {}
    Value(bool b) : type(Type::Boolean), boolean(b) {}
    Value(Boolean b) : Value(b.value) {}
    Value(Number::type x) : type(Type::Number), number(x) {}
    Value(Number x) : Value(x.value) {}
    Value(String s) : type(Type::String), string(std::move(s)) {}
    Value(Block b) : type(Type::Block), block(b.address) {}

    Value(const Value& other);
    Value(Value&& other) noexcept;
    Value& operator=(const Value& other);
    Value& operator=(Value&& other) noexcept;
    ~Value();

    Value& swap(Value& other) noexcept;

    bool is_null() const noexcept { return type == Type::Null; }
    bool is_bool() const noexcept { return type == Type::Boolean; }
    bool is_number() const noexcept { return type == Type::Number; }
    bool is_string() const noexcept { return type == Type::String; }

    Boolean to_bool() const;
    Number to_number() const;
    String to_string() const&;
    String to_string() &&;
    Block to_block() const;

    friend bool operator==(const Value& lhs, const Value& rhs) noexcept {
      if (lhs.type != rhs.type)
        return false;
      switch (lhs.type) {
      case Type::Null:
        return true;
      case Type::Boolean:
        return lhs.boolean == rhs.boolean;
      case Type::Number:
        return lhs.number == rhs.number;
      case Type::Block:
        return lhs.block == rhs.block;
      case Type::String:
        return lhs.string == rhs.string;
      }
      return false;
    }

    friend std::ostream& operator<<(std::ostream& os, const Value& value) {
      switch (value.type) {
      case Type::Null:
        return os << "Null()";
      case Type::Boolean:
        return os << "Boolean(" << (value.boolean ? "true" : "false") << ")";
      case Type::Number:
        return os << "Number(" << value.number << ")";
      case Type::String:
        return os << "String(" << value.string << ")";
      case Type::Block:
        return os << "Function(" << value.block << ")";
      }
      return os;
    }

  private:
    Type type;
    union {
      bool boolean;
      Number::type number;
      String string;
      std::size_t block;
    };
  };

}

#endif // KNIGHT_VALUE_HPP_INCLUDED
