#ifndef KNIGHT_FUNCS_HPP_INCLUDED
#define KNIGHT_FUNCS_HPP_INCLUDED

#include <cstddef>
#include <string>
#include "eval.hpp"

namespace kn::funcs {

  std::string open_shell(std::string command);

  // control flow
  std::size_t no_op(const kn::eval::CodePoint* bytecode, std::size_t offset);
  std::size_t error(const kn::eval::CodePoint* bytecode, std::size_t offset);
  std::size_t call(const kn::eval::CodePoint* bytecode, std::size_t offset);
  std::size_t return_(const kn::eval::CodePoint* bytecode, std::size_t offset);
  std::size_t jump(const kn::eval::CodePoint* bytecode, std::size_t offset);
  std::size_t jump_if(const kn::eval::CodePoint* bytecode, std::size_t offset);
  std::size_t jump_if_not(const kn::eval::CodePoint* bytecode, std::size_t offset);

  // arithmetic
  std::size_t plus(const kn::eval::CodePoint* bytecode, std::size_t offset);
  std::size_t minus(const kn::eval::CodePoint* bytecode, std::size_t offset);
  std::size_t multiplies(const kn::eval::CodePoint* bytecode, std::size_t offset);
  std::size_t divides(const kn::eval::CodePoint* bytecode, std::size_t offset);
  std::size_t modulus(const kn::eval::CodePoint* bytecode, std::size_t offset);
  std::size_t exponent(const kn::eval::CodePoint* bytecode, std::size_t offset);

  // logical
  std::size_t negate(const kn::eval::CodePoint* bytecode, std::size_t offset);
  std::size_t less(const kn::eval::CodePoint* bytecode, std::size_t offset);
  std::size_t greater(const kn::eval::CodePoint* bytecode, std::size_t offset);
  std::size_t equals(const kn::eval::CodePoint* bytecode, std::size_t offset);

  // string
  std::size_t length(const kn::eval::CodePoint* bytecode, std::size_t offset);
  std::size_t get(const kn::eval::CodePoint* bytecode, std::size_t offset);
  std::size_t substitute(const kn::eval::CodePoint* bytecode, std::size_t offset);

  // environment
  std::size_t assign(const kn::eval::CodePoint* bytecode, std::size_t offset);
  std::size_t prompt(const kn::eval::CodePoint* bytecode, std::size_t offset);
  std::size_t output(const kn::eval::CodePoint* bytecode, std::size_t offset);
  std::size_t random(const kn::eval::CodePoint* bytecode, std::size_t offset);
  std::size_t shell(const kn::eval::CodePoint* bytecode, std::size_t offset);
  std::size_t quit(const kn::eval::CodePoint* bytecode, std::size_t offset);
  std::size_t eval(const kn::eval::CodePoint* bytecode, std::size_t offset);
  std::size_t dump(const kn::eval::CodePoint* bytecode, std::size_t offset);

}

#endif // KNIGHT_FUNCS_HPP_INCLUDED
