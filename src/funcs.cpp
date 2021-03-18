#include "funcs.hpp"

#include <cassert>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <random>
#include <vector>

#include "env.hpp"
#include "error.hpp"
#include "value.hpp"

using namespace kn::eval;

namespace {

  Value get_value(CodePoint cp) {
    switch (cp.label.cat()) {
    case LabelCat::Constant:
      return Number(cp.label.id());
    case LabelCat::JumpTarget:
      return Block{ cp.label.id() };
    case LabelCat::Variable:
      return Environment::get().value(cp.label);
    case LabelCat::Unused:
      throw kn::Error("error: read placeholder value while parsing");
    }
#ifdef __GNUC__
    __builtin_unreachable();
#endif
  }

  void set_result(const CodePoint* bytecode, std::size_t offset, Value v) {
    Environment::get().assign(bytecode[offset + 1].label, std::move(v));
  }

  template <typename F>
  std::size_t binary_math_op(const CodePoint* bytecode, std::size_t offset, F f) {
    auto lhs = get_value(bytecode[offset + 2]);
    if (lhs.is_number()) {
      auto x = lhs.to_number();
      auto y = get_value(bytecode[offset + 3]).to_number();
      set_result(bytecode, offset, f(x, y));
    }

    return offset + 4;
  }

  template <typename F>
  std::size_t binary_compare_op(const CodePoint* bytecode, std::size_t offset, F f) {
    auto lhs = get_value(bytecode[offset + 2]);
    if (lhs.is_number()) {
      auto x = lhs.to_number();
      auto y = get_value(bytecode[offset + 3]).to_number();
      set_result(bytecode, offset, f(x, y));
    } else if (lhs.is_number()) {
      auto x = lhs.to_string();
      auto y = get_value(bytecode[offset + 3]).to_string();
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

  struct CallStack {
    std::size_t caller;
    Label result;
  };
  std::vector<CallStack> call_stack;

}

namespace kn::funcs {

  // control flow

  std::size_t no_op(const CodePoint*, std::size_t offset) {
    return offset + 1;
  }

  std::size_t error(const CodePoint* bytecode, std::size_t offset) {
    using namespace std::literals;
    throw kn::Error("error executing OpCode="s +
                    std::to_string(static_cast<std::size_t>(bytecode[offset].op)) +
                    " at offset="s +
                    std::to_string(offset));
  }

  std::size_t call(const CodePoint* bytecode, std::size_t offset) {
    assert(bytecode[offset].op == OpCode::Call);
    call_stack.push_back({ offset, bytecode[offset + 1].label });
    return get_value(bytecode[offset + 2]).to_block().address;
  }

  std::size_t return_(const CodePoint* bytecode, std::size_t offset) {
    assert(bytecode[offset].op == OpCode::Return);
    auto frame = call_stack.back();
    call_stack.pop_back();
    Environment::get().assign(frame.result, get_value(bytecode[offset + 1]));
    return frame.caller + 3;
  }

  std::size_t jump(const CodePoint* bytecode, std::size_t offset) {
    assert(bytecode[offset].op == OpCode::Jump);
    assert(bytecode[offset + 1].label.cat() == LabelCat::JumpTarget);
    return bytecode[offset + 1].label.id();
  }

  std::size_t jump_if(const CodePoint* bytecode, std::size_t offset) {
    assert(bytecode[offset].op == OpCode::JumpIf);
    assert(bytecode[offset + 1].label.cat() == LabelCat::JumpTarget);

    if (get_value(bytecode[offset + 2]).to_bool())
      return bytecode[offset + 1].label.id();
    return offset + 3;
  }

  std::size_t jump_if_not(const CodePoint* bytecode, std::size_t offset) {
    assert(bytecode[offset].op == OpCode::JumpIfNot);
    assert(bytecode[offset + 1].label.cat() == LabelCat::JumpTarget);

    if (not get_value(bytecode[offset + 2]).to_bool())
      return bytecode[offset + 1].label.id();
    return offset + 3;
  }

  // arithmetic
  std::size_t plus(const CodePoint* bytecode, std::size_t offset) {
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

  std::size_t multiplies(const CodePoint* bytecode, std::size_t offset) {
    assert(bytecode[offset].op == OpCode::Plus);

    auto lhs = get_value(bytecode[offset + 2]);
    if (lhs.is_number()) {
      auto x = lhs.to_number();
      auto y = get_value(bytecode[offset + 3]).to_number();
      set_result(bytecode, offset, x * y);
    } else if (lhs.is_string()) {
      auto x = lhs.to_string();
      auto y = static_cast<std::size_t>(get_value(bytecode[offset + 3]).to_number());
      auto s = std::string{};
      s.reserve(x.size() * y);
      while (y--) s += x;
      set_result(bytecode, offset, std::move(s));
    } else {
      assert(false);
    }

    return offset + 4;
  }

  std::size_t minus(const CodePoint* bytecode, std::size_t offset) {
    assert(bytecode[offset].op == OpCode::Minus);
    return binary_math_op(bytecode, offset, std::minus{});
  }

  std::size_t divides(const CodePoint* bytecode, std::size_t offset) {
    assert(bytecode[offset].op == OpCode::Divides);
    return binary_math_op(bytecode, offset, std::divides{});
  }

  std::size_t modulus(const CodePoint* bytecode, std::size_t offset) {
    assert(bytecode[offset].op == OpCode::Modulus);
    return binary_math_op(bytecode, offset, std::modulus{});
  }

  std::size_t exponent(const CodePoint* bytecode, std::size_t offset) {
    assert(bytecode[offset].op == OpCode::Exponent);
    return binary_math_op(bytecode, offset, [](Number lhs, Number rhs) {
      return static_cast<Number>(std::pow(lhs, rhs)); });
  }

  // logical

  std::size_t negate(const CodePoint* bytecode, std::size_t offset) {
    assert(bytecode[offset].op == OpCode::Negate);
    auto val = not get_value(bytecode[offset + 2]).to_bool();
    set_result(bytecode, offset, val);
    return offset + 3;
  }

  std::size_t less(const CodePoint* bytecode, std::size_t offset) {
    assert(bytecode[offset].op == OpCode::Less);
    return binary_compare_op(bytecode, offset, std::less{});
  }

  std::size_t greater(const CodePoint* bytecode, std::size_t offset) {
    assert(bytecode[offset].op == OpCode::Greater);
    return binary_compare_op(bytecode, offset, std::greater{});
  }

  std::size_t equals(const CodePoint* bytecode, std::size_t offset) {
    assert(bytecode[offset].op == OpCode::Equals);
    auto lhs = get_value(bytecode[offset + 2]);
    auto rhs = get_value(bytecode[offset + 3]);
    set_result(bytecode, offset, lhs == rhs);
    return offset + 4;
  }

  // string
  std::size_t length(const CodePoint* bytecode, std::size_t offset) {
    assert(bytecode[offset].op == OpCode::Length);
    auto str = get_value(bytecode[offset + 2]).to_string();
    set_result(bytecode, offset, static_cast<Number>(str.length()));
    return offset + 3;
  }

  std::size_t get(const CodePoint* bytecode, std::size_t offset) {
    assert(bytecode[offset].op == OpCode::Get);
    auto str = get_value(bytecode[offset + 2]).to_string();
    auto pos = static_cast<std::size_t>(get_value(bytecode[offset + 3]).to_number());
    auto len = static_cast<std::size_t>(get_value(bytecode[offset + 4]).to_number());
    set_result(bytecode, offset, str.substr(pos, len));
    return offset + 5;
  }

  std::size_t substitute(const CodePoint* bytecode, std::size_t offset) {
    assert(bytecode[offset].op == OpCode::Substitute);
    auto str = get_value(bytecode[offset + 2]).to_string();
    auto pos = static_cast<std::size_t>(get_value(bytecode[offset + 3]).to_number());
    auto len = static_cast<std::size_t>(get_value(bytecode[offset + 4]).to_number());
    auto replace = get_value(bytecode[offset + 5]).to_string();
    set_result(bytecode, offset, str.replace(pos, len, replace));
    return offset + 6;
  }

  // environment

  std::size_t assign(const CodePoint* bytecode, std::size_t offset) {
    assert(bytecode[offset].op == OpCode::Assign);
    set_result(bytecode, offset, get_value(bytecode[offset + 2]));
    return offset + 3;
  }

  std::size_t prompt(const CodePoint* bytecode, std::size_t offset) {
    assert(bytecode[offset].op == OpCode::Prompt);
    auto line = std::string{};
    std::getline(std::cin, line);
    set_result(bytecode, offset, std::move(line));
    return offset + 2;
  }

  std::size_t output(const CodePoint* bytecode, std::size_t offset) {
    assert(bytecode[offset].op == OpCode::Output);
    auto str = get_value(bytecode[offset + 1]).to_string();
    if (not str.empty() and str.back() == '\\') {
      str.pop_back();
      std::cout << str << std::flush;
    } else {
      std::cout << str << '\n' << std::flush;
    }
    return offset + 2;
  }

  std::size_t random(const CodePoint* bytecode, std::size_t offset) {
    assert(bytecode[offset].op == OpCode::Random);
    thread_local auto generator = std::mt19937(std::random_device{}());
    thread_local auto dist = std::uniform_int_distribution(
      Number{}, std::numeric_limits<Number>::max());
    set_result(bytecode, offset, dist(generator));
    return offset + 2;
  }

  std::size_t shell(const CodePoint* bytecode, std::size_t offset) {
    assert(bytecode[offset].op == OpCode::Shell);
    set_result(bytecode, offset,
               open_shell(get_value(bytecode[offset + 2]).to_string()));
    return offset + 3;
  }

  std::size_t quit(const CodePoint* bytecode, std::size_t offset) {
    assert(bytecode[offset].op == OpCode::Quit);
    std::exit(get_value(bytecode[offset + 1]).to_number());
  }

  std::size_t eval(const CodePoint* bytecode, std::size_t offset) {
    assert(bytecode[offset].op == OpCode::Eval);
    // TODO
  }

  std::size_t dump(const CodePoint* bytecode, std::size_t offset) {
    assert(bytecode[offset].op == OpCode::Dump);
    std::cout << get_value(bytecode[offset + 1]);
    return offset + 2;
  }

}

