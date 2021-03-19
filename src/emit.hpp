#ifndef KNIGHT_EMIT_HPP_INCLUDED
#define KNIGHT_EMIT_HPP_INCLUDED

#include "parser.hpp"

namespace kn::parser::emit {

  // arity 0

  Emitted true_(ASTFrame ast, ParseInfo& info);
  Emitted false_(ASTFrame ast, ParseInfo& info);
  Emitted null(ASTFrame ast, ParseInfo& info);
  Emitted prompt(ASTFrame ast, ParseInfo& info);
  Emitted random(ASTFrame ast, ParseInfo& info);

  // arity 1

  Emitted eval(ASTFrame ast, ParseInfo& info);
  Emitted block(ASTFrame ast, ParseInfo& info);
  Emitted call(ASTFrame ast, ParseInfo& info);
  Emitted shell(ASTFrame ast, ParseInfo& info);
  Emitted quit(ASTFrame ast, ParseInfo& info);
  Emitted negate(ASTFrame ast, ParseInfo& info);
  Emitted length(ASTFrame ast, ParseInfo& info);
  Emitted dump(ASTFrame ast, ParseInfo& info);
  Emitted output(ASTFrame ast, ParseInfo& info);

  // arity 2

  Emitted plus(ASTFrame ast, ParseInfo& info);
  Emitted minus(ASTFrame ast, ParseInfo& info);
  Emitted multiplies(ASTFrame ast, ParseInfo& info);
  Emitted divides(ASTFrame ast, ParseInfo& info);
  Emitted modulus(ASTFrame ast, ParseInfo& info);
  Emitted exponent(ASTFrame ast, ParseInfo& info);
  Emitted less(ASTFrame ast, ParseInfo& info);
  Emitted greater(ASTFrame ast, ParseInfo& info);
  Emitted equals(ASTFrame ast, ParseInfo& info);
  Emitted conjunct(ASTFrame ast, ParseInfo& info);
  Emitted disjunct(ASTFrame ast, ParseInfo& info);
  Emitted sequence(ASTFrame ast, ParseInfo& info);
  Emitted assign(ASTFrame ast, ParseInfo& info);
  Emitted while_(ASTFrame ast, ParseInfo& info);

  // arity 3

  Emitted ifelse(ASTFrame ast, ParseInfo& info);
  Emitted get(ASTFrame ast, ParseInfo& info);

  // arity 4

  Emitted substitute(ASTFrame ast, ParseInfo& info);
}

#endif // KNIGHT_EMIT_HPP_INCLUDED
