#include "eval.hpp"

#include <array>
#include <cassert>
#include <deque>
#include <utility>
#include <vector>
#include <unordered_map>

#ifdef KN_HAS_DEBUGGER
#include <cstring>
#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#endif

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
    { 1, kn::funcs::error },    // block_label
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
  ByteCode prepare(const std::vector<Operation>& program, std::size_t label_offset) {
    // our new list
    // potentially overreserve, but we're not super worried about that generally
    // (the average instruction has one opcode and two labels)
    auto rewritten = std::vector<CodePoint>{};
    rewritten.reserve(3 * program.size());

    // map from local offset -> label ID
    auto local_offsets = std::vector<std::pair<std::size_t, std::size_t>>{};

    for (const auto& op : program) switch (op.op) {
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
    for (auto&& [x, id] : local_offsets) {
      if (auto it = labels.find(id); it != labels.end())
        rewritten[x] = CodePoint(Label(LabelCat::JumpTarget, it->second));
    }

    return rewritten;
  }

  void run(ByteCode program) {
    // make sure we have a "finish" at the end of the program
    auto end_pos = program.size();
    auto retval = Environment::get().get_variable("#retval");
    program.emplace_back(OpCode::Quit);
    program.emplace_back(retval);

    // set up the stack frame
    assert(program[0].op == OpCode::BlockData);
    Environment::get().push_frame(end_pos, retval, program[1].label.id());

    // ignore the block data at the start of the program
    std::size_t offset = 2;  // TODO: make this a global constant

    // run until we stop
    while (offset < program.size()) {
      auto op = program[offset].op;
      offset = get_function(op)(program, offset);
    }
  }

#ifdef KN_HAS_DEBUGGER
  namespace {

    std::ostream& print_label(std::ostream& os, Label l) {
      switch (l.cat()) {
      case LabelCat::Constant:
        return os << "$" << l.id();
      case LabelCat::Variable:
        return os << "{" << Environment::get().nameof(l) << "}";
      case LabelCat::Temporary:
        return os << "[t:" << l.id() << "]";
      case LabelCat::Literal:
        return os << Environment::get().value(l);
      case LabelCat::JumpTarget:
        return os << ">" << l.id();
      default:
        return os << "!!";
      }
    }

    std::ostream& print_opcode(std::ostream& os, OpCode op) {
      switch (op) {
      case kn::eval::OpCode::NoOp:
        return os << "0   ";
      case kn::eval::OpCode::Label:
        return os << "#l  ";
      case kn::eval::OpCode::BlockData:
        return os << "#b  ";
      case kn::eval::OpCode::Call:
        return os << "cl  ";
      case kn::eval::OpCode::Return:
        return os << "ret ";
      case kn::eval::OpCode::Jump:
        return os << "jmp ";
      case kn::eval::OpCode::JumpIf:
        return os << "jy  ";
      case kn::eval::OpCode::JumpIfNot:
        return os << "jn  ";

        // arithmetic
      case kn::eval::OpCode::Plus:
        return os << "add ";
      case kn::eval::OpCode::Minus:
        return os << "sub ";
      case kn::eval::OpCode::Multiplies:
        return os << "mul ";
      case kn::eval::OpCode::Divides:
        return os << "div ";
      case kn::eval::OpCode::Modulus:
        return os << "mod ";
      case kn::eval::OpCode::Exponent:
        return os << "exp ";

        // logical
      case kn::eval::OpCode::Negate:
        return os << "neg ";
      case kn::eval::OpCode::Less:
        return os << "lt  ";
      case kn::eval::OpCode::Greater:
        return os << "gt  ";
      case kn::eval::OpCode::Equals:
        return os << "eq  ";

        // string
      case kn::eval::OpCode::Length:
        return os << "len ";
      case kn::eval::OpCode::Get:
        return os << "get ";
      case kn::eval::OpCode::Substitute:
        return os << "sub ";

        // environment
      case kn::eval::OpCode::Assign:
        return os << "=   ";
      case kn::eval::OpCode::Prompt:
        return os << "inp ";
      case kn::eval::OpCode::Output:
        return os << "out ";
      case kn::eval::OpCode::Random:
        return os << "rnd ";
      case kn::eval::OpCode::Shell:
        return os << "sh  ";
      case kn::eval::OpCode::Quit:
        return os << "q   ";
      case kn::eval::OpCode::Eval:
        return os << "evl ";
      case kn::eval::OpCode::Dump:
        return os << "dmp ";

      case kn::eval::OpCode::NumberOfOps:
        break;
      }
      return os;
    }

    void print_whole_program(std::ostream& os, std::size_t curr, std::size_t brk, const ByteCode& program) {
      for (std::size_t offset = 0; offset < program.size(); ++offset) {
        if (offset == curr) os << '>';
        else if (offset == brk) os << '!';
        else os << ' ';
        os << std::setw(5) << offset << ": ";
        print_opcode(os, program[offset].op);
        for (int i = 0; i < get_num_labels(program[offset].op); ++i)
          print_label(os, program[offset + (std::size_t)i + 1].label) << ' ';
        offset += (std::size_t)get_num_labels(program[offset].op);
        os << '\n';
      }
    }

  }

  void debug(ByteCode program) {
    // make sure we have a "finish" at the end of the program
    auto end_pos = program.size();
    auto retval = Environment::get().get_variable("#retval");
    program.emplace_back(OpCode::Quit);
    program.emplace_back(retval);

    // set up the stack frame
    assert(program[0].op == OpCode::BlockData);
    Environment::get().push_frame(end_pos, retval, program[1].label.id());

    std::size_t offset = 2;  // see comment in `run`
    std::size_t old_size = program.size();
    std::size_t breakpoint = -1;

    std::cout << std::right;
    std::cout << "assembled:\n";
    print_whole_program(std::cout, offset, breakpoint, program);

    std::cout << "\n\nStarting debugging:\n\n";
    while (offset < program.size()) {
      auto op = program[offset].op;
      std::cout << std::setw(4) << offset << "[";
      print_opcode(std::cout, op);
      std::cout << "]> ";

      std::string inp;
      if (not std::getline(std::cin, inp)) {
        std::cout << "exit.\n";
        std::exit(0);
      }
      // entering nothing goes to the next statement
      if (inp.empty())
        inp = "n";

      if (inp[0] == 'l') {
        print_whole_program(std::cout, offset, breakpoint, program);
      } else if (inp[0] == 'p') {
        std::string command;
        std::size_t varid;
        std::istringstream iss(inp);
        iss >> command;
        while (iss >> varid) {
          auto label = Label(LabelCat::Temporary, varid);
          std::cout << "[t:" << varid << "] => ";
          if (Environment::get().has_value(label))
            std::cout << Environment::get().value(label) << '\n';
          else
            std::cout << "#empty\n";
        }
      } else if (inp[0] == 'n') {
        offset = get_function(op)(program, offset);
        if (program.size() != old_size) {
          old_size = program.size();
        }
      } else if (inp[0] == 'c') {
        while (offset < program.size() and offset != breakpoint) {
          offset = get_function(program[offset].op)(program, offset);
          if (program.size() != old_size) old_size = program.size();
        }
      } else if (inp[0] == 'd') {
        Environment::get().dump_vars();
      } else if (inp[0] == 'b') {
        std::istringstream iss(inp);
        iss >> inp >> breakpoint;
        std::cout << "set breakpoint.\n";
      } else if (inp[0] == 'r') {
        for (auto&& cp : program) {
          std::size_t x = 0;
          assert(sizeof x == sizeof cp);
          std::memcpy(&x, &cp, sizeof cp);
          std::cout << std::hex << x << ' ';
        }
        std::cout << '\n';
      } else if (inp[0] == 'q') {
        break;
      }
    }
  }
#endif

}
