#ifndef KNIGHT_PARSER_HPP_INCLUDED
#define KNIGHT_PARSER_HPP_INCLUDED

#include <cstddef>
#include <memory>
#include <vector>

#include "value.hpp"
#include "lexer.hpp"

namespace kn {

  struct Expression {
    virtual ~Expression();
    virtual kn::eval::Value evaluate() const = 0;
  };
  using ExpressionPtr = std::unique_ptr<Expression>;

  // information about the current stage of parsing
  struct ParseInfo {
    // the name of the current token
    // (mutable, steps through the token stream)
    lexer::TokenIter* iter;
    // the root token for the current expression
    lexer::TokenIter curr_expr;
    // sentinel for the end of the token stream
    lexer::TokenIter end;

    const lexer::Token& self() const { return **iter; }
  };

  ExpressionPtr parse(const std::vector<lexer::Token>& tokens);
  ExpressionPtr parse_arg(const ParseInfo& info);

}

#endif  // KNIGHT_PARSER_HPP_INCLUDED
