#include "eval.hpp"

#include <array>
#include <cassert>
#include <deque>
#include <utility>
#include <vector>
#include <unordered_map>

#include "env.hpp"
#include "funcs.hpp"
#include "parser.hpp"

namespace {

  using namespace kn::eval;

  using op_table = std::array<
    std::pair<int, std::size_t(*)(ByteCode& bytecode, std::size_t offset)>,
    static_cast<std::size_t>(OpCode::NumberOfOps)>;

  // TODO: don't split this very particular important information
  // over like three different files :s
  inline constexpr auto op_funcs = op_table{{
    { 0, kn::funcs::no_op },
    { 1, kn::funcs::error },    // label
    { 2, kn::funcs::call },
    { 1, kn::funcs::return_ },
    { 1, kn::funcs::jump },
    { 2, kn::funcs::jump_if },
    { 2, kn::funcs::jump_if_not },
    { 3, kn::funcs::plus },
    { 3, kn::funcs::minus },
    { 3, kn::funcs::multiplies },
    { 3, kn::funcs::divides },
    { 3, kn::funcs::modulus },
    { 3, kn::funcs::exponent },
    { 2, kn::funcs::negate },
    { 3, kn::funcs::less },
    { 3, kn::funcs::greater },
    { 3, kn::funcs::equals },
    { 2, kn::funcs::length },
    { 4, kn::funcs::get },
    { 5, kn::funcs::substitute },
    { 2, kn::funcs::assign },
    { 1, kn::funcs::prompt },
    { 1, kn::funcs::output },
    { 1, kn::funcs::random },
    { 2, kn::funcs::shell },
    { 1, kn::funcs::quit },
    { 2, kn::funcs::eval },
    { 1, kn::funcs::dump },
  }};
  constexpr int get_num_labels(OpCode op) noexcept {
    return op_funcs[static_cast<std::size_t>(op)].first;
  }
  constexpr auto get_function(OpCode op) noexcept {
    return op_funcs[static_cast<std::size_t>(op)].second;
  }

  // map from label ID -> label offset
  auto labels = std::unordered_map<std::size_t, std::size_t>{};

}

namespace kn::eval {

  // prepare the instructions for execution:
  // remove labels, determine jump offsets, and flatten structure
  ByteCode prepare(const parser::Emitted& program, std::size_t label_offset) {
    const auto& instructions = program.instructions;

    // our new list
    // potentially overreserve, but we're not super worried about that generally
    // (the average instruction has one opcode and two labels)
    auto rewritten = std::vector<CodePoint>{};
    rewritten.reserve(3 * instructions.size());

    // map from local offset -> label ID
    auto local_offsets = std::vector<std::pair<std::size_t, std::size_t>>{};

    for (const auto& op : instructions) switch (op.op) {
      case OpCode::Label: {
        assert(op.labels[0].cat() == LabelCat::JumpTarget);
        [[maybe_unused]] auto [_, s] = labels.try_emplace(
          op.labels[0].id(), rewritten.size() + label_offset);
        assert(s);
      } break;

      case OpCode::Call: {
        rewritten.emplace_back(op.op);
        assert(op.labels[0].cat() != LabelCat::JumpTarget);
        rewritten.emplace_back(op.labels[0]);
        if (op.labels[1].cat() == LabelCat::JumpTarget) {
          local_offsets.emplace_back(rewritten.size(), op.labels[1].id());
          rewritten.emplace_back(Label{});  // placeholder
        } else {
          rewritten.emplace_back(op.labels[1]);
        }
      } break;

      case OpCode::Jump:
      case OpCode::JumpIf:
      case OpCode::JumpIfNot: {
        rewritten.emplace_back(op.op);
        assert(op.labels[0].cat() == LabelCat::JumpTarget);
        local_offsets.emplace_back(rewritten.size(), op.labels[0].id());
        rewritten.emplace_back(Label{});  // placeholder
        if (op.op != OpCode::Jump) {
          assert(op.labels[1].cat() != LabelCat::JumpTarget);
          rewritten.emplace_back(op.labels[1]);
        }
      } break;

      default: {
        rewritten.emplace_back(op.op);
        for (int i = 0; i < get_num_labels(op.op); ++i) {
          if (op.labels[i].cat() == LabelCat::JumpTarget) {
            local_offsets.emplace_back(rewritten.size(), op.labels[i].id());
            rewritten.emplace_back(Label{});  // placeholder
          } else {
            rewritten.emplace_back(op.labels[i]);
          }
        }
      } break;
    }

    // now we do our mapping back into the offset table
    for (auto [x, id] : local_offsets) {
      if (auto it = labels.find(id); it != labels.end())
        rewritten[x] = CodePoint(Label(LabelCat::JumpTarget, it->second));
    }

    return rewritten;
  }

  void run(ByteCode program) {
    // make sure we have a "finish" at the end of the program
    program.emplace_back(OpCode::Quit);
    program.emplace_back(Label::from_constant(0));

    std::size_t offset = 0;
    while (offset < program.size()) {
      auto op = program[offset].op;
      offset = get_function(op)(program, offset);
    }
  }

}
