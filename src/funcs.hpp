#ifndef KNIGHT_FUNCS_HPP_INCLUDED
#define KNIGHT_FUNCS_HPP_INCLUDED

#include "parser.hpp"

namespace kn::funcs {

  using lexer::TokenIter;

  // arity 0

  ExpressionPtr true_(ParseInfo info);
  ExpressionPtr false_(ParseInfo info);
  ExpressionPtr null(ParseInfo info);
  ExpressionPtr prompt(ParseInfo info);
  ExpressionPtr random(ParseInfo info);

  // arity 1

  ExpressionPtr eval(ParseInfo info);
  ExpressionPtr block(ParseInfo info);
  ExpressionPtr call(ParseInfo info);
  ExpressionPtr shell(ParseInfo info);
  ExpressionPtr quit(ParseInfo info);
  ExpressionPtr negate(ParseInfo info);
  ExpressionPtr length(ParseInfo info);
  ExpressionPtr dump(ParseInfo info);
  ExpressionPtr output(ParseInfo info);

  // arity 2

  ExpressionPtr plus(ParseInfo info);
  ExpressionPtr minus(ParseInfo info);
  ExpressionPtr multiplies(ParseInfo info);
  ExpressionPtr divides(ParseInfo info);
  ExpressionPtr modulus(ParseInfo info);
  ExpressionPtr exponent(ParseInfo info);
  ExpressionPtr less(ParseInfo info);
  ExpressionPtr greater(ParseInfo info);
  ExpressionPtr equals(ParseInfo info);
  ExpressionPtr conjunct(ParseInfo info);
  ExpressionPtr disjunct(ParseInfo info);
  ExpressionPtr sequence(ParseInfo info);
  ExpressionPtr assign(ParseInfo info);
  ExpressionPtr while_(ParseInfo info);

  // arity 3

  ExpressionPtr ifelse(ParseInfo info);
  ExpressionPtr get(ParseInfo info);

  // arity 4

  ExpressionPtr substitute(ParseInfo info);
}

#endif // KNIGHT_FUNCS_HPP_INCLUDED
