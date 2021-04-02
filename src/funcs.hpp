#ifndef KNIGHT_FUNCS_HPP_INCLUDED
#define KNIGHT_FUNCS_HPP_INCLUDED

#include <cstddef>
#include <string>
#include "eval.hpp"

namespace kn::funcs {

  std::string open_shell(const std::string& command);

  // control flow
  std::size_t no_op(kn::eval::ByteCode& bytecode, std::size_t offset);
  std::size_t error(kn::eval::ByteCode& bytecode, std::size_t offset);
  std::size_t call(kn::eval::ByteCode& bytecode, std::size_t offset);
  std::size_t return_(kn::eval::ByteCode& bytecode, std::size_t offset);
  std::size_t jump(kn::eval::ByteCode& bytecode, std::size_t offset);
  std::size_t jump_if(kn::eval::ByteCode& bytecode, std::size_t offset);
  std::size_t jump_if_not(kn::eval::ByteCode& bytecode, std::size_t offset);

  // arithmetic
  std::size_t plus(kn::eval::ByteCode& bytecode, std::size_t offset);
  std::size_t minus(kn::eval::ByteCode& bytecode, std::size_t offset);
  std::size_t multiplies(kn::eval::ByteCode& bytecode, std::size_t offset);
  std::size_t divides(kn::eval::ByteCode& bytecode, std::size_t offset);
  std::size_t modulus(kn::eval::ByteCode& bytecode, std::size_t offset);
  std::size_t exponent(kn::eval::ByteCode& bytecode, std::size_t offset);

  // logical
  std::size_t negate(kn::eval::ByteCode& bytecode, std::size_t offset);
  std::size_t less(kn::eval::ByteCode& bytecode, std::size_t offset);
  std::size_t greater(kn::eval::ByteCode& bytecode, std::size_t offset);
  std::size_t equals(kn::eval::ByteCode& bytecode, std::size_t offset);

  // string
  std::size_t length(kn::eval::ByteCode& bytecode, std::size_t offset);
  std::size_t get(kn::eval::ByteCode& bytecode, std::size_t offset);
  std::size_t substitute(kn::eval::ByteCode& bytecode, std::size_t offset);

  // environment
  std::size_t assign(kn::eval::ByteCode& bytecode, std::size_t offset);
  std::size_t prompt(kn::eval::ByteCode& bytecode, std::size_t offset);
  std::size_t output(kn::eval::ByteCode& bytecode, std::size_t offset);
  std::size_t random(kn::eval::ByteCode& bytecode, std::size_t offset);
  std::size_t shell(kn::eval::ByteCode& bytecode, std::size_t offset);
  std::size_t quit(kn::eval::ByteCode& bytecode, std::size_t offset);
  std::size_t eval(kn::eval::ByteCode& bytecode, std::size_t offset);
  std::size_t dump(kn::eval::ByteCode& bytecode, std::size_t offset);

}

#endif // KNIGHT_FUNCS_HPP_INCLUDED
