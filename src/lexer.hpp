#ifndef KNIGHT_LEXER_HPP_INCLUDED
#define KNIGHT_LEXER_HPP_INCLUDED

#include <ostream>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>
#include "sourcepos.hpp"

namespace kn::lexer {

  struct StringLiteral {
    std::string_view data;
    friend std::ostream& operator<<(std::ostream& os, StringLiteral s) {
      return os << "STRING_LITERAL(" << s.data << ")";
    }
  };
  struct NumericLiteral {
    int data;
    friend std::ostream& operator<<(std::ostream& os, NumericLiteral n) {
      return os << "NUMERIC_LITERAL(" << n.data << ")";
    }
  };
  struct Identifier {
    std::string_view name;
    friend std::ostream& operator<<(std::ostream& os, Identifier i) {
      return os << "IDENTIFIER(" << i.name << ")";
    }
  };
  struct Function {
    char id;
    friend std::ostream& operator<<(std::ostream& os, Function f) {
      return os << "FUNCTION(" << f.id << ")";
    }
  };

  class Token {
  public:
    Token(StringLiteral data, SourcePosition first, SourcePosition last) noexcept
      : m_data(data), m_first(first), m_last(last)
    {}
    Token(NumericLiteral data, SourcePosition first, SourcePosition last) noexcept
      : m_data(data), m_first(first), m_last(last)
    {}
    Token(Identifier data, SourcePosition first, SourcePosition last) noexcept
      : m_data(data), m_first(first), m_last(last)
    {}
    Token(Function data, SourcePosition first, SourcePosition last) noexcept
      : m_data(data), m_first(first), m_last(last)
    {}

    template <typename T>
    Token(T data, SourcePosition pos) noexcept
      : Token(std::move(data), pos, pos)
    {}

    SourcePosition pos() const noexcept { return m_first; }

    std::pair<SourcePosition, SourcePosition> range() const noexcept {
      return { m_first, m_last };
    }

    friend std::ostream& operator<<(std::ostream& os, const Token& tok) {
      std::visit([&os](auto&& t) { os << t; }, tok.m_data);
      os << " @ " << tok.m_first.to_string();
      if (tok.m_first != tok.m_last)
        os << "#" << tok.m_last.to_string();
      return os;
    }

    const StringLiteral* as_string_lit() const noexcept {
      return std::get_if<StringLiteral>(&m_data);
    }
    const NumericLiteral* as_numeric_lit() const noexcept {
      return std::get_if<NumericLiteral>(&m_data);
    }
    const Identifier* as_ident() const noexcept {
      return std::get_if<Identifier>(&m_data);
    }
    const Function* as_function() const noexcept {
      return std::get_if<Function>(&m_data);
    }

  private:
    std::variant<StringLiteral, NumericLiteral, Identifier, Function> m_data;
    SourcePosition m_first;
    SourcePosition m_last;
  };

  std::vector<Token> tokenise(std::string_view str);
  using TokenIter = std::vector<Token>::const_iterator;

}

#endif  // KNIGHT_LEXER_HPP_INCLUDED
