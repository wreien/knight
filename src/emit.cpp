#include "emit.hpp"

#include <cassert>

#include "env.hpp"
#include "eval.hpp"

using namespace kn::eval;
using namespace kn::parser;
using env = kn::eval::Environment;

namespace {

  Label new_temp() { return env::get().get_temp(); }
  Label new_label() { return env::get().get_jump(); }

  Emitted gen_onearg(ASTFrame&& ast, OpCode op) {
    assert(ast.arity == 1);
    auto& x = ast.children[0];

    auto result = new_temp();
    x.instructions.emplace_back(op, result, x.result);
    return { result, std::move(x.instructions) };
  }

  Emitted gen_onearg_noreturn(ASTFrame&& ast, OpCode op) {
    assert(ast.arity == 1);
    auto& x = ast.children[0];

    auto result = env::get().get_literal(Null{});
    x.instructions.emplace_back(op, x.result);
    return { result, std::move(x.instructions) };
  }

  Emitted gen_twoarg(ASTFrame&& ast, OpCode op) {
    assert(ast.arity == 2);
    auto& lhs = ast.children[0];
    auto& rhs = ast.children[1];

    auto result = new_temp();
    lhs.instructions.insert(
      lhs.instructions.end(),
      rhs.instructions.begin(), rhs.instructions.end());
    lhs.instructions.emplace_back(op, result, lhs.result, rhs.result);
    return { result, std::move(lhs.instructions) };
  }

  // for conjunct and disjunt;
  // brancher is one of JumpIf or JumpIfNot
  Emitted shortcircuit_logic(ASTFrame&& ast, OpCode brancher) {
    assert(ast.arity == 2);
    auto& lhs = ast.children[0];
    auto& rhs = ast.children[1];

    auto finish = new_label();
    auto result = new_temp();

    lhs.instructions.emplace_back(OpCode::Assign, result, lhs.result);
    lhs.instructions.emplace_back(brancher, finish, lhs.result);
    lhs.instructions.insert(
      lhs.instructions.end(),
      rhs.instructions.begin(), rhs.instructions.end());
    // assign back the result onto the same variable
    lhs.instructions.emplace_back(OpCode::Assign, result, rhs.result);
    lhs.instructions.emplace_back(OpCode::Label, finish);
    return { result, std::move(lhs.instructions) };
  }
}

namespace kn::parser::emit {


  // arity 0


  Emitted true_([[maybe_unused]] ASTFrame ast) {
    assert(ast.arity == 0);
    return { env::get().get_literal(true) };
  }

  Emitted false_([[maybe_unused]] ASTFrame ast) {
    assert(ast.arity == 0);
    return { env::get().get_literal(false) };
  }

  Emitted null([[maybe_unused]] ASTFrame ast) {
    assert(ast.arity == 0);
    return { env::get().get_literal(Null{}) };
  }

  Emitted prompt([[maybe_unused]] ASTFrame ast) {
    assert(ast.arity == 0);
    auto result = new_temp();
    return { result, { Operation(OpCode::Prompt, result) } };
  }

  Emitted random([[maybe_unused]] ASTFrame ast) {
    assert(ast.arity == 0);
    auto result = new_temp();
    return { result, { Operation(OpCode::Random, result) } };
  }


  // arity 1


  Emitted block(ASTFrame ast) {
    assert(ast.arity == 1);
    auto& x = ast.children[0];

    // TODO: extract out block and remove the jump

    auto entry_point = new_label();
    auto exit_point  = new_label();

    x.instructions.emplace_front(OpCode::Label, entry_point);
    x.instructions.emplace_front(OpCode::Jump, exit_point);

    x.instructions.emplace_back(OpCode::Return, x.result);
    x.instructions.emplace_back(OpCode::Label, exit_point);

    return { entry_point, std::move(x.instructions) };
  }

  Emitted eval(ASTFrame ast) {
    return gen_onearg(std::move(ast), OpCode::Eval); }
  Emitted call(ASTFrame ast) {
    return gen_onearg(std::move(ast), OpCode::Call); }
  Emitted shell(ASTFrame ast) {
    return gen_onearg(std::move(ast), OpCode::Shell); }
  Emitted negate(ASTFrame ast) {
    return gen_onearg(std::move(ast), OpCode::Negate); }
  Emitted length(ASTFrame ast) {
    return gen_onearg(std::move(ast), OpCode::Length); }

  Emitted output(ASTFrame ast) {
    return gen_onearg_noreturn(std::move(ast), OpCode::Output); }
  Emitted dump(ASTFrame ast) {
    return gen_onearg_noreturn(std::move(ast), OpCode::Dump); }
  Emitted quit(ASTFrame ast) {
    return gen_onearg_noreturn(std::move(ast), OpCode::Quit); }


  // arity 2


  Emitted assign(ASTFrame ast) {
    assert(ast.arity == 2);

    // must be an identifier, always no instructions
    auto var = ast.children[0].result;
    assert(ast.children[0].instructions.empty());

    auto& x = ast.children[1];
    x.instructions.emplace_back(OpCode::Assign, var, x.result);
    return { var, std::move(x.instructions) };
  }

  Emitted sequence(ASTFrame ast) {
    assert(ast.arity == 2);
    auto& lhs = ast.children[0];
    auto& rhs = ast.children[1];

    lhs.instructions.insert(
      lhs.instructions.end(),
      rhs.instructions.begin(), rhs.instructions.end());
    return { rhs.result, std::move(lhs.instructions) };
  }

  Emitted while_(ASTFrame ast) {
    assert(ast.arity == 2);
    auto& cond = ast.children[0];
    auto& loop = ast.children[1];

    auto start = new_label();
    auto finish = new_label();

    cond.instructions.emplace_front(OpCode::Label, start);
    cond.instructions.emplace_back(OpCode::JumpIfNot, finish, cond.result);
    cond.instructions.insert(
      cond.instructions.end(),
      loop.instructions.begin(), loop.instructions.end());
    cond.instructions.emplace_back(OpCode::Jump, start);
    cond.instructions.emplace_back(OpCode::Label, finish);
    return { env::get().get_literal(Null{}), std::move(cond.instructions) };
  }

  Emitted plus(ASTFrame ast) {
    return gen_twoarg(std::move(ast), OpCode::Plus); }
  Emitted minus(ASTFrame ast) {
    return gen_twoarg(std::move(ast), OpCode::Minus); }
  Emitted multiplies(ASTFrame ast) {
    return gen_twoarg(std::move(ast), OpCode::Multiplies); }
  Emitted divides(ASTFrame ast) {
    return gen_twoarg(std::move(ast), OpCode::Divides); }
  Emitted modulus(ASTFrame ast) {
    return gen_twoarg(std::move(ast), OpCode::Modulus); }
  Emitted exponent(ASTFrame ast) {
    return gen_twoarg(std::move(ast), OpCode::Exponent); }
  Emitted less(ASTFrame ast) {
    return gen_twoarg(std::move(ast), OpCode::Less); }
  Emitted greater(ASTFrame ast) {
    return gen_twoarg(std::move(ast), OpCode::Greater); }
  Emitted equals(ASTFrame ast) {
    return gen_twoarg(std::move(ast), OpCode::Equals); }

  Emitted disjunct(ASTFrame ast) {
    return shortcircuit_logic(std::move(ast), OpCode::JumpIf); }
  Emitted conjunct(ASTFrame ast) {
    return shortcircuit_logic(std::move(ast), OpCode::JumpIfNot); }


  // arity 3


  Emitted ifelse(ASTFrame ast) {
    assert(ast.arity == 3);
    auto& cond = ast.children[0];
    auto& yes = ast.children[1];
    auto& no = ast.children[2];

    auto no_label = new_label();
    auto end_label = new_label();
    auto result = new_temp();

    cond.instructions.emplace_back(OpCode::JumpIfNot, no_label, cond.result);

    // true case
    cond.instructions.insert(
      cond.instructions.end(),
      yes.instructions.begin(), yes.instructions.end());
    cond.instructions.emplace_back(OpCode::Assign, result, yes.result);
    cond.instructions.emplace_back(OpCode::Jump, end_label);

    cond.instructions.emplace_back(OpCode::Label, no_label);

    // false case
    cond.instructions.insert(
      cond.instructions.end(),
      no.instructions.begin(), no.instructions.end());
    cond.instructions.emplace_back(OpCode::Assign, result, no.result);
    cond.instructions.emplace_back(OpCode::Label, end_label);

    return { result, std::move(cond.instructions) };
  }

  Emitted get(ASTFrame ast) {
    assert(ast.arity == 3);
    auto& str = ast.children[0];
    auto& pos = ast.children[1];
    auto& len = ast.children[2];

    auto result = new_temp();

    str.instructions.insert(
      str.instructions.end(),
      pos.instructions.begin(), pos.instructions.end());
    str.instructions.insert(
      str.instructions.end(),
      len.instructions.begin(), len.instructions.end());

    str.instructions.emplace_back(
      OpCode::Get, result, str.result, pos.result, len.result);
    return { result, std::move(str.instructions) };
  }


  // arity 4


  Emitted substitute(ASTFrame ast) {
    assert(ast.arity == 4);
    auto& str = ast.children[0];
    auto& pos = ast.children[1];
    auto& len = ast.children[2];
    auto& replace = ast.children[3];

    auto result = new_temp();

    str.instructions.insert(
      str.instructions.end(),
      pos.instructions.begin(), pos.instructions.end());
    str.instructions.insert(
      str.instructions.end(),
      len.instructions.begin(), len.instructions.end());
    str.instructions.insert(
      str.instructions.end(),
      replace.instructions.begin(), replace.instructions.end());

    str.instructions.emplace_back(
      OpCode::Substitute, result,
      str.result, pos.result, len.result, replace.result);
    return { result, std::move(str.instructions) };
  }

}
