#include "funcs.hpp"

#include <cassert>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <random>
#include <unordered_map>
#include <vector>

#include "env.hpp"
#include "error.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "value.hpp"

using namespace kn::eval;

namespace {

  const Value& get_value(CodePoint cp) {
    static Value to_return;
    switch (cp.label.cat()) {
    case LabelCat::Constant:
      return (to_return = Number(cp.label.id()));
    case LabelCat::JumpTarget:
      return (to_return = Block{ cp.label.id() });
    case LabelCat::Literal:
    case LabelCat::Temporary:
    case LabelCat::Variable:
      return Environment::get().value(cp.label);
    case LabelCat::Unused:
      throw kn::Error("error: read placeholder value while evaluating");
    }
#ifdef __GNUC__
    __builtin_unreachable();
#endif
  }

  void set_result(ByteCode& bytecode, std::size_t offset, Value v) {
    Environment::get().assign(bytecode[offset + 1].label, std::move(v));
  }

  template <typename F>
  std::size_t binary_math_op(ByteCode& bytecode, std::size_t offset, F f) {
    auto lhs = get_value(bytecode[offset + 2]);
    if (lhs.is_number()) {
      auto x = lhs.to_number();
      auto y = get_value(bytecode[offset + 3]).to_number();
      set_result(bytecode, offset, f(x, y));
    }

    return offset + 4;
  }

  template <typename F>
  std::size_t binary_compare_op(ByteCode& bytecode, std::size_t offset, F f) {
    auto lhs = get_value(bytecode[offset + 2]);
    if (lhs.is_number()) {
      auto x = lhs.to_number();
      auto y = get_value(bytecode[offset + 3]).to_number();
      set_result(bytecode, offset, f(x, y));
    } else if (lhs.is_string()) {
      const auto& x = lhs.to_string().as_str();
      const auto& y = get_value(bytecode[offset + 3]).to_string().as_str();
      set_result(bytecode, offset, f(x, y));
    } else if (lhs.is_bool()) {
      auto x = lhs.to_bool();
      auto y = get_value(bytecode[offset + 3]).to_bool();
      set_result(bytecode, offset, f(x, y));
    } else {
      assert(false);
    }
    return offset + 4;
  }

  std::unordered_map<std::string, std::size_t> evals;

}

namespace kn::funcs {

  // control flow

  std::size_t no_op(ByteCode&, std::size_t offset) {
    return offset + 1;
  }

  std::size_t error(ByteCode& bytecode, std::size_t offset) {
    using namespace std::literals;
    throw kn::Error("error executing OpCode="s +
                    std::to_string(static_cast<std::size_t>(bytecode[offset].op)) +
                    " at offset="s +
                    std::to_string(offset));
  }

  std::size_t call(ByteCode& bytecode, std::size_t offset) {
    assert(bytecode[offset].op == OpCode::Call);

    // ensure call structure is correct
    auto dest = get_value(bytecode[offset + 2]).to_block().address;
    assert(bytecode[dest - 2].op == OpCode::BlockData);
    auto num_temps = bytecode[dest - 1].label.id();

    // bump the call stack
    Environment::get().push_frame(offset + 3, bytecode[offset + 1].label, num_temps);

    // and do a jump to the subroutine
    return dest;
  }

  std::size_t return_(ByteCode& bytecode, std::size_t offset) {
    assert(bytecode[offset].op == OpCode::Return);

    // we're leaving a frame, bump the call stack
    auto value = get_value(bytecode[offset + 1]);
    auto [retaddr, result] = Environment::get().pop_frame();
    Environment::get().assign(result, std::move(value));

    // return to sender
    return retaddr;
  }

  std::size_t jump(ByteCode& bytecode, std::size_t offset) {
    assert(bytecode[offset].op == OpCode::Jump);
    assert(bytecode[offset + 1].label.cat() == LabelCat::JumpTarget);
    return bytecode[offset + 1].label.id();
  }

  std::size_t jump_if(ByteCode& bytecode, std::size_t offset) {
    assert(bytecode[offset].op == OpCode::JumpIf);
    assert(bytecode[offset + 1].label.cat() == LabelCat::JumpTarget);

    if (get_value(bytecode[offset + 2]).to_bool())
      return bytecode[offset + 1].label.id();
    return offset + 3;
  }

  std::size_t jump_if_not(ByteCode& bytecode, std::size_t offset) {
    assert(bytecode[offset].op == OpCode::JumpIfNot);
    assert(bytecode[offset + 1].label.cat() == LabelCat::JumpTarget);

    if (not get_value(bytecode[offset + 2]).to_bool())
      return bytecode[offset + 1].label.id();
    return offset + 3;
  }

  // arithmetic
  std::size_t plus(ByteCode& bytecode, std::size_t offset) {
    assert(bytecode[offset].op == OpCode::Plus);

    auto lhs = get_value(bytecode[offset + 2]);
    if (lhs.is_number()) {
      auto x = lhs.to_number();
      auto y = get_value(bytecode[offset + 3]).to_number();
      set_result(bytecode, offset, x + y);
    } else if (lhs.is_string()) {
      auto x = lhs.to_string();
      auto y = get_value(bytecode[offset + 3]).to_string();
      set_result(bytecode, offset, x + y);
    } else {
      assert(false);
    }

    return offset + 4;
  }

  std::size_t multiplies(ByteCode& bytecode, std::size_t offset) {
    assert(bytecode[offset].op == OpCode::Multiplies);

    auto lhs = get_value(bytecode[offset + 2]);
    if (lhs.is_number()) {
      auto x = lhs.to_number();
      auto y = get_value(bytecode[offset + 3]).to_number();
      set_result(bytecode, offset, x * y);
    } else if (lhs.is_string()) {
      auto x = lhs.to_string();
      auto y = get_value(bytecode[offset + 3]).to_number();
      set_result(bytecode, offset, x * y);
    } else {
      assert(false);
    }

    return offset + 4;
  }

  std::size_t minus(ByteCode& bytecode, std::size_t offset) {
    assert(bytecode[offset].op == OpCode::Minus);
    return binary_math_op(bytecode, offset, std::minus{});
  }

  std::size_t divides(ByteCode& bytecode, std::size_t offset) {
    assert(bytecode[offset].op == OpCode::Divides);
    return binary_math_op(bytecode, offset, std::divides{});
  }

  std::size_t modulus(ByteCode& bytecode, std::size_t offset) {
    assert(bytecode[offset].op == OpCode::Modulus);
    return binary_math_op(bytecode, offset, std::modulus{});
  }

  std::size_t exponent(ByteCode& bytecode, std::size_t offset) {
    assert(bytecode[offset].op == OpCode::Exponent);
    return binary_math_op(bytecode, offset, [](Number lhs, Number rhs) {
      return static_cast<Number>(std::pow(lhs.value, rhs.value)); });
  }

  // logical

  std::size_t negate(ByteCode& bytecode, std::size_t offset) {
    assert(bytecode[offset].op == OpCode::Negate);
    auto val = not get_value(bytecode[offset + 2]).to_bool();
    set_result(bytecode, offset, val);
    return offset + 3;
  }

  std::size_t less(ByteCode& bytecode, std::size_t offset) {
    assert(bytecode[offset].op == OpCode::Less);
    return binary_compare_op(bytecode, offset, std::less{});
  }

  std::size_t greater(ByteCode& bytecode, std::size_t offset) {
    assert(bytecode[offset].op == OpCode::Greater);
    return binary_compare_op(bytecode, offset, std::greater{});
  }

  std::size_t equals(ByteCode& bytecode, std::size_t offset) {
    assert(bytecode[offset].op == OpCode::Equals);
    auto lhs = get_value(bytecode[offset + 2]);
    auto rhs = get_value(bytecode[offset + 3]);
    set_result(bytecode, offset, lhs == rhs);
    return offset + 4;
  }

  // string
  std::size_t length(ByteCode& bytecode, std::size_t offset) {
    assert(bytecode[offset].op == OpCode::Length);
    auto str = get_value(bytecode[offset + 2]).to_string();
    set_result(bytecode, offset, static_cast<Number>(str.size()));
    return offset + 3;
  }

  std::size_t get(ByteCode& bytecode, std::size_t offset) {
    assert(bytecode[offset].op == OpCode::Get);
    auto str = get_value(bytecode[offset + 2]).to_string();
    auto pos = static_cast<std::size_t>(get_value(bytecode[offset + 3]).to_number());
    auto len = static_cast<std::size_t>(get_value(bytecode[offset + 4]).to_number());
    set_result(bytecode, offset, str.substr(pos, len));
    return offset + 5;
  }

  std::size_t substitute(ByteCode& bytecode, std::size_t offset) {
    assert(bytecode[offset].op == OpCode::Substitute);
    auto str = get_value(bytecode[offset + 2]).to_string();
    auto pos = static_cast<std::size_t>(get_value(bytecode[offset + 3]).to_number());
    auto len = static_cast<std::size_t>(get_value(bytecode[offset + 4]).to_number());
    auto replace = get_value(bytecode[offset + 5]).to_string();
    set_result(bytecode, offset, str.replace(pos, len, replace));
    return offset + 6;
  }

  // environment

  std::size_t assign(ByteCode& bytecode, std::size_t offset) {
    assert(bytecode[offset].op == OpCode::Assign);
    set_result(bytecode, offset, get_value(bytecode[offset + 2]));
    return offset + 3;
  }

  std::size_t prompt(ByteCode& bytecode, std::size_t offset) {
    assert(bytecode[offset].op == OpCode::Prompt);
    auto line = std::string{};
    std::getline(std::cin, line);
    set_result(bytecode, offset, String(std::move(line)));
    return offset + 2;
  }

  std::size_t output(ByteCode& bytecode, std::size_t offset) {
    assert(bytecode[offset].op == OpCode::Output);
    auto str = get_value(bytecode[offset + 1]).to_string();
    str.output(std::cout);
    return offset + 2;
  }

  std::size_t random(ByteCode& bytecode, std::size_t offset) {
    assert(bytecode[offset].op == OpCode::Random);
    thread_local auto generator = std::mt19937(std::random_device{}());
    thread_local auto dist = std::uniform_int_distribution(
      Number::type{}, std::numeric_limits<Number::type>::max());
    set_result(bytecode, offset, dist(generator));
    return offset + 2;
  }

  std::size_t shell(ByteCode& bytecode, std::size_t offset) {
    assert(bytecode[offset].op == OpCode::Shell);
    const auto& str = get_value(bytecode[offset + 2]).to_string().as_str();
    set_result(bytecode, offset, String(open_shell(str)));
    return offset + 3;
  }

  std::size_t quit(ByteCode& bytecode, std::size_t offset) {
    assert(bytecode[offset].op == OpCode::Quit);
    std::exit(get_value(bytecode[offset + 1]).to_number());
  }

  // TODO: rewrite eval?
  std::size_t eval(ByteCode& bytecode, std::size_t offset) {
    assert(bytecode[offset].op == OpCode::Eval);
    auto next_statement = offset + 3;
    auto result = bytecode[offset + 1].label;

    // parse input and generate parsetree
    const auto& input = get_value(bytecode[offset + 2]).to_string().as_str();
    if (auto it = evals.find(input); it != evals.end()) {
      Environment::get().push_frame(
        offset + 3,
        bytecode[offset + 1].label,
        bytecode[it->second - 1].label.id());
      return it->second;
    }

    auto tokens = kn::lexer::tokenise(input);
    auto program = kn::parser::parse(tokens);

    // store offsets and prepare new bytecode
    auto new_offset = bytecode.size() + 2;  // see `eval::run`
    auto new_bytecode = kn::eval::prepare(program, bytecode.size());

    // get block data and construct new stack frame
    assert(new_bytecode[0].op == OpCode::BlockData);
    Environment::get().push_frame(
      next_statement, result, new_bytecode[1].label.id());

    // add the bytecode to the current execution set
    bytecode.insert(bytecode.end(), new_bytecode.begin(), new_bytecode.end());

    // cache the string so we don't need to parse this one again
    evals.emplace(std::move(input), new_offset);

    // return the start of the newly evaluated bytecode
    return new_offset;
  }

  std::size_t dump(ByteCode& bytecode, std::size_t offset) {
    assert(bytecode[offset].op == OpCode::Dump);
    std::cout << get_value(bytecode[offset + 1]);
    return offset + 2;
  }

}

