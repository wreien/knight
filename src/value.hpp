#ifndef KNIGHT_VALUE_HPP_INCLUDED
#define KNIGHT_VALUE_HPP_INCLUDED

#include <memory>
#include <ostream>
#include <string>
#include <variant>

namespace kn::eval {

  struct Value;

  // expressions
  struct Expression {
    virtual ~Expression();
    virtual Value evaluate() const = 0;
    virtual void dump() const = 0;
  };
  using ExpressionPtr = std::unique_ptr<const Expression>;

  // basic types
  using Boolean = bool;
  using Number = int;
  using String = std::string;
  struct Null {
    friend bool operator==(Null, Null) noexcept { return true; }
  };
  struct Block {
    friend bool operator==(Block, Block) noexcept { return false; }
    Block(ExpressionPtr expr) : expr(expr.release()) {}
    std::shared_ptr<const Expression> expr;
  };

  struct Value : std::variant<Null, Boolean, Number, String, Block> {
    using variant::variant;

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
      else
        os << "???";
      return os;
    }
  };

}

#endif // KNIGHT_VALUE_HPP_INCLUDED
