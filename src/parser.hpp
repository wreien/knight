#ifndef KNIGHT_PARSER_HPP_INCLUDED
#define KNIGHT_PARSER_HPP_INCLUDED

#include <cassert>
#include <cstddef>
#include <memory>
#include <vector>

#include "value.hpp"
#include "lexer.hpp"

namespace kn {

  struct Expression {
    virtual ~Expression();
    virtual eval::Value evaluate() const = 0;
    virtual void dump() const = 0;
  };
  using ExpressionPtr = std::unique_ptr<Expression>;

  inline constexpr std::size_t max_arity = 4;

  // information about the current stage of parsing
  struct ParseInfo {
    // place in the token stream
    lexer::TokenIter token;

    // the id of the function to call on completion
    std::size_t func_id;

    // args to be used to construct the given expression
    ExpressionPtr args[max_arity];

    // number of args expected vs. filled in
    int arity;
    int num_args;

    void add_arg(ExpressionPtr expr) noexcept {
      assert(num_args < arity);
      args[num_args++] = std::move(expr);
    }
    bool is_completed() const noexcept {
      return num_args == arity;
    }
  };

  ExpressionPtr parse(const std::vector<lexer::Token>& tokens);

}

#endif  // KNIGHT_PARSER_HPP_INCLUDED
