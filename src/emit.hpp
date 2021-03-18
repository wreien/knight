#ifndef KNIGHT_EMIT_HPP_INCLUDED
#define KNIGHT_EMIT_HPP_INCLUDED

#include "parser.hpp"

namespace kn::parser::emit {

  // arity 0

  Emitted true_(ASTFrame ast);
  Emitted false_(ASTFrame ast);
  Emitted null(ASTFrame ast);
  Emitted prompt(ASTFrame ast);
  Emitted random(ASTFrame ast);

  // arity 1

  Emitted eval(ASTFrame ast);
  Emitted block(ASTFrame ast);
  Emitted call(ASTFrame ast);
  Emitted shell(ASTFrame ast);
  Emitted quit(ASTFrame ast);
  Emitted negate(ASTFrame ast);
  Emitted length(ASTFrame ast);
  Emitted dump(ASTFrame ast);
  Emitted output(ASTFrame ast);

  // arity 2

  Emitted plus(ASTFrame ast);
  Emitted minus(ASTFrame ast);
  Emitted multiplies(ASTFrame ast);
  Emitted divides(ASTFrame ast);
  Emitted modulus(ASTFrame ast);
  Emitted exponent(ASTFrame ast);
  Emitted less(ASTFrame ast);
  Emitted greater(ASTFrame ast);
  Emitted equals(ASTFrame ast);
  Emitted conjunct(ASTFrame ast);
  Emitted disjunct(ASTFrame ast);
  Emitted sequence(ASTFrame ast);
  Emitted assign(ASTFrame ast);
  Emitted while_(ASTFrame ast);

  // arity 3

  Emitted ifelse(ASTFrame ast);
  Emitted get(ASTFrame ast);

  // arity 4

  Emitted substitute(ASTFrame ast);
}

#endif // KNIGHT_EMIT_HPP_INCLUDED
