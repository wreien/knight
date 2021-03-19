#include "emit.hpp"

#include <iostream>
#include <cassert>

#include "env.hpp"
#include "eval.hpp"

using namespace kn::eval;
using namespace kn::parser;
using env = kn::eval::Environment;

namespace {

  Emitted& cache_expr(Emitted& expr, ParseInfo& info) {
    if (expr.result.cat() == LabelCat::Variable) {
      auto cache = info.new_temp();
      expr.instructions.emplace_back(OpCode::Assign, cache, expr.result);
      expr.result = cache;
    }
    return expr;
  }

  Emitted gen_onearg(ASTFrame&& ast, ParseInfo& info, OpCode op) {
    assert(ast.arity == 1);
    auto& x = ast.children[0];

    auto result = info.new_temp();
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

  Emitted gen_twoarg(ASTFrame&& ast, ParseInfo& info, OpCode op) {
    assert(ast.arity == 2);

    // if the result variable is mutable, make sure to cache it
    // so that we don't break evaluation order
    auto& lhs = cache_expr(ast.children[0], info);
    auto& rhs = ast.children[1];

    auto result = info.new_temp();
    lhs.instructions.insert(
      lhs.instructions.end(),
      rhs.instructions.begin(), rhs.instructions.end());
    lhs.instructions.emplace_back(op, result, lhs.result, rhs.result);
    return { result, std::move(lhs.instructions) };
  }

  // for conjunct and disjunt;
  // brancher is one of JumpIf or JumpIfNot
  Emitted shortcircuit_logic(ASTFrame&& ast, ParseInfo& info, OpCode brancher) {
    assert(ast.arity == 2);
    auto& lhs = ast.children[0];
    auto& rhs = ast.children[1];

    auto finish = info.new_jump();
    auto result = info.new_temp();

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


  Emitted true_([[maybe_unused]] ASTFrame ast, ParseInfo&) {
    assert(ast.arity == 0);
    return { env::get().get_literal(true) };
  }

  Emitted false_([[maybe_unused]] ASTFrame ast, ParseInfo&) {
    assert(ast.arity == 0);
    return { env::get().get_literal(false) };
  }

  Emitted null([[maybe_unused]] ASTFrame ast, ParseInfo&) {
    assert(ast.arity == 0);
    return { env::get().get_literal(Null{}) };
  }

  Emitted prompt([[maybe_unused]] ASTFrame ast, ParseInfo& info) {
    assert(ast.arity == 0);
    auto result = info.new_temp();
    return { result, { Operation(OpCode::Prompt, result) } };
  }

  Emitted random([[maybe_unused]] ASTFrame ast, ParseInfo& info) {
    assert(ast.arity == 0);
    auto result = info.new_temp();
    return { result, { Operation(OpCode::Random, result) } };
  }


  // arity 1


  Emitted block(ASTFrame ast, ParseInfo& info) {
    assert(ast.arity == 1);
    auto& x = ast.children[0];

    auto entry_point = info.new_jump();
    auto num_temps = Label::from_constant(info.pop_frame());

    // block structure:
    //   blocksize
    //   label
    //   ...
    //   return

    x.instructions.emplace_front(OpCode::Label, entry_point);
    x.instructions.emplace_back(OpCode::Return, x.result);

    x.instructions.emplace_front(OpCode::BlockData, num_temps);

    info.blocks.emplace_back(std::move(x.instructions));

    return { entry_point, {} };
  }

  Emitted eval(ASTFrame ast, ParseInfo& info) {
    return gen_onearg(std::move(ast), info, OpCode::Eval); }
  Emitted call(ASTFrame ast, ParseInfo& info) {
    return gen_onearg(std::move(ast), info, OpCode::Call); }
  Emitted shell(ASTFrame ast, ParseInfo& info) {
    return gen_onearg(std::move(ast), info, OpCode::Shell); }
  Emitted negate(ASTFrame ast, ParseInfo& info) {
    return gen_onearg(std::move(ast), info, OpCode::Negate); }
  Emitted length(ASTFrame ast, ParseInfo& info) {
    return gen_onearg(std::move(ast), info, OpCode::Length); }

  Emitted output(ASTFrame ast, ParseInfo&) {
    return gen_onearg_noreturn(std::move(ast), OpCode::Output); }
  Emitted dump(ASTFrame ast, ParseInfo&) {
    return gen_onearg_noreturn(std::move(ast), OpCode::Dump); }
  Emitted quit(ASTFrame ast, ParseInfo&) {
    return gen_onearg_noreturn(std::move(ast), OpCode::Quit); }


  // arity 2


  Emitted assign(ASTFrame ast, ParseInfo&) {
    assert(ast.arity == 2);

    // must be an identifier, always no instructions
    auto var = ast.children[0].result;
    for (auto&& i : ast.children[0].instructions) {
      std::cout << static_cast<int>(i.op) << std::endl;
    }
    assert(ast.children[0].instructions.empty());

    auto& x = ast.children[1];
    x.instructions.emplace_back(OpCode::Assign, var, x.result);
    return { var, std::move(x.instructions) };
  }

  Emitted sequence(ASTFrame ast, ParseInfo&) {
    assert(ast.arity == 2);
    auto& lhs = ast.children[0];
    auto& rhs = ast.children[1];

    lhs.instructions.insert(
      lhs.instructions.end(),
      rhs.instructions.begin(), rhs.instructions.end());
    return { rhs.result, std::move(lhs.instructions) };
  }

  Emitted while_(ASTFrame ast, ParseInfo& info) {
    assert(ast.arity == 2);
    auto& cond = ast.children[0];
    auto& loop = ast.children[1];

    auto start = info.new_jump();
    auto finish = info.new_jump();

    cond.instructions.emplace_front(OpCode::Label, start);
    cond.instructions.emplace_back(OpCode::JumpIfNot, finish, cond.result);
    cond.instructions.insert(
      cond.instructions.end(),
      loop.instructions.begin(), loop.instructions.end());
    cond.instructions.emplace_back(OpCode::Jump, start);
    cond.instructions.emplace_back(OpCode::Label, finish);
    return { env::get().get_literal(Null{}), std::move(cond.instructions) };
  }

  Emitted plus(ASTFrame ast, ParseInfo& info) {
    return gen_twoarg(std::move(ast), info, OpCode::Plus); }
  Emitted minus(ASTFrame ast, ParseInfo& info) {
    return gen_twoarg(std::move(ast), info, OpCode::Minus); }
  Emitted multiplies(ASTFrame ast, ParseInfo& info) {
    return gen_twoarg(std::move(ast), info, OpCode::Multiplies); }
  Emitted divides(ASTFrame ast, ParseInfo& info) {
    return gen_twoarg(std::move(ast), info, OpCode::Divides); }
  Emitted modulus(ASTFrame ast, ParseInfo& info) {
    return gen_twoarg(std::move(ast), info, OpCode::Modulus); }
  Emitted exponent(ASTFrame ast, ParseInfo& info) {
    return gen_twoarg(std::move(ast), info, OpCode::Exponent); }
  Emitted less(ASTFrame ast, ParseInfo& info) {
    return gen_twoarg(std::move(ast), info, OpCode::Less); }
  Emitted greater(ASTFrame ast, ParseInfo& info) {
    return gen_twoarg(std::move(ast), info, OpCode::Greater); }
  Emitted equals(ASTFrame ast, ParseInfo& info) {
    return gen_twoarg(std::move(ast), info, OpCode::Equals); }

  Emitted disjunct(ASTFrame ast, ParseInfo& info) {
    return shortcircuit_logic(std::move(ast), info, OpCode::JumpIf); }
  Emitted conjunct(ASTFrame ast, ParseInfo& info) {
    return shortcircuit_logic(std::move(ast), info, OpCode::JumpIfNot); }


  // arity 3


  Emitted ifelse(ASTFrame ast, ParseInfo& info) {
    assert(ast.arity == 3);
    auto& cond = ast.children[0];
    auto& yes = ast.children[1];
    auto& no = ast.children[2];

    auto no_label = info.new_jump();
    auto end_label = info.new_jump();
    auto result = info.new_temp();

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

  Emitted get(ASTFrame ast, ParseInfo& info) {
    assert(ast.arity == 3);

    // handle mutable result variables and instruction reordering
    auto& str = cache_expr(ast.children[0], info);
    auto& pos = cache_expr(ast.children[1], info);
    auto& len = ast.children[2];

    auto result = info.new_temp();

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


  Emitted substitute(ASTFrame ast, ParseInfo& info) {
    assert(ast.arity == 4);

    // handle mutable result variables and instruction reordering
    auto& str = cache_expr(ast.children[0], info);
    auto& pos = cache_expr(ast.children[1], info);
    auto& len = cache_expr(ast.children[2], info);
    auto& replace = ast.children[3];

    auto result = info.new_temp();

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
