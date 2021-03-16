#ifndef KNIGHT_FUNCS_HPP_INCLUDED
#define KNIGHT_FUNCS_HPP_INCLUDED

#include "parser.hpp"

namespace kn::funcs {

  using lexer::TokenIter;

  // arity 0

  ExpressionPtr true_(const ParseInfo& info);
  ExpressionPtr false_(const ParseInfo& info);
  ExpressionPtr null(const ParseInfo& info);
  ExpressionPtr prompt(const ParseInfo& info);
  ExpressionPtr random(const ParseInfo& info);

  // arity 1

  ExpressionPtr eval(const ParseInfo& info);
  ExpressionPtr block(const ParseInfo& info);
  ExpressionPtr call(const ParseInfo& info);
  ExpressionPtr shell(const ParseInfo& info);
  ExpressionPtr quit(const ParseInfo& info);
  ExpressionPtr negate(const ParseInfo& info);
  ExpressionPtr length(const ParseInfo& info);
  ExpressionPtr dump(const ParseInfo& info);
  ExpressionPtr output(const ParseInfo& info);

  // arity 2

  ExpressionPtr plus(const ParseInfo& info);
  ExpressionPtr minus(const ParseInfo& info);
  ExpressionPtr multiplies(const ParseInfo& info);
  ExpressionPtr divides(const ParseInfo& info);
  ExpressionPtr modulus(const ParseInfo& info);
  ExpressionPtr exponent(const ParseInfo& info);
  ExpressionPtr less(const ParseInfo& info);
  ExpressionPtr greater(const ParseInfo& info);
  ExpressionPtr identity(const ParseInfo& info);
  ExpressionPtr conjunct(const ParseInfo& info);
  ExpressionPtr disjunct(const ParseInfo& info);
  ExpressionPtr sequence(const ParseInfo& info);
  ExpressionPtr assign(const ParseInfo& info);
  ExpressionPtr while_(const ParseInfo& info);

  // arity 3

  ExpressionPtr ifelse(const ParseInfo& info);
  ExpressionPtr get(const ParseInfo& info);

  // arity 4

  ExpressionPtr substitute(const ParseInfo& info);
}

#endif // KNIGHT_FUNCS_HPP_INCLUDED
