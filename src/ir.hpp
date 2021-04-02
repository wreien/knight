#ifndef KNIGHT_IR_HPP_INCLUDED
#define KNIGHT_IR_HPP_INCLUDED

#include "eval.hpp"
#include "parser.hpp"

namespace kn::ir {

  std::vector<eval::Operation> optimise(const parser::Block& block);
  std::vector<eval::Operation> optimise(const std::vector<parser::Block>& blocks);

}

#endif // KNIGHT_IR_HPP_INCLUDED
