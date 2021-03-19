#ifndef KNIGHT_PARSER_HPP_INCLUDED
#define KNIGHT_PARSER_HPP_INCLUDED

#include <cassert>
#include <cstddef>
#include <memory>
#include <vector>
#include <deque>

#include "value.hpp"
#include "lexer.hpp"
#include "eval.hpp"

namespace kn::parser {

  inline constexpr std::size_t max_arity = 4;

  struct Emitted {
    Emitted() = default;
    Emitted(eval::Label result) : result(result) {}
    Emitted(eval::Label result, std::deque<eval::Operation> instructions)
      : result(result), instructions(std::move(instructions))
    {}

    eval::Label result = {};
    std::deque<eval::Operation> instructions = {};
  };

  // general parsing stuff
  struct ParseInfo {
    std::vector<std::deque<eval::Operation>> blocks = {};
    std::vector<std::size_t> temp_stack = { 0 };
    std::size_t jump_labels = 0;

    void push_frame() {
      temp_stack.emplace_back(0);
    }
    std::size_t pop_frame() noexcept {
      auto num_temps = temp_stack.back();
      temp_stack.pop_back();
      return num_temps;
    }
    eval::Label new_temp() noexcept {
      assert(not temp_stack.empty());
      return { eval::LabelCat::Temporary, temp_stack.back()++ };
    }
    eval::Label new_jump() noexcept {
      return { eval::LabelCat::JumpTarget, jump_labels++ };
    }
  };

  // information about the current stage of parsing
  struct ASTFrame {
    // the operation this will perform
    std::size_t func;

    // children of this AST
    Emitted children[max_arity];

    // number of args expected vs. filled in
    int arity;
    int num_args;

    void add_child(Emitted child) noexcept {
      assert(num_args < arity);
      children[num_args++] = std::move(child);
    }
    bool is_completed() const noexcept {
      return num_args == arity;
    }
  };

  Emitted parse(const std::vector<lexer::Token>& tokens);

}

#endif  // KNIGHT_PARSER_HPP_INCLUDED
