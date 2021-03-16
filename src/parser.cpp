#include "parser.hpp"

#include "env.hpp"
#include "error.hpp"
#include "funcs.hpp"
#include "lexer.hpp"
#include "value.hpp"

#include <array>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

using kn::lexer::TokenIter;

namespace {

  using parse_fn = kn::eval::ExpressionPtr(kn::ParseInfo);
  using function_table = std::array<
    std::pair<int, parse_fn*>,
    std::numeric_limits<unsigned char>::max()>;

  auto functions = []() -> function_table {
    function_table table = {};

    table['T'] = { 0, kn::funcs::true_ };
    table['F'] = { 0, kn::funcs::false_ };
    table['N'] = { 0, kn::funcs::null };
    table['P'] = { 0, kn::funcs::prompt };
    table['R'] = { 0, kn::funcs::random };

    table['E'] = { 1, kn::funcs::eval };
    table['B'] = { 1, kn::funcs::block };
    table['C'] = { 1, kn::funcs::call };
    table['`'] = { 1, kn::funcs::shell };
    table['Q'] = { 1, kn::funcs::quit };
    table['!'] = { 1, kn::funcs::negate };
    table['L'] = { 1, kn::funcs::length };
    table['D'] = { 1, kn::funcs::dump };
    table['O'] = { 1, kn::funcs::output };

    table['+'] = { 2, kn::funcs::plus };
    table['-'] = { 2, kn::funcs::minus };
    table['*'] = { 2, kn::funcs::multiplies };
    table['/'] = { 2, kn::funcs::divides };
    table['%'] = { 2, kn::funcs::modulus };
    table['^'] = { 2, kn::funcs::exponent };
    table['<'] = { 2, kn::funcs::less };
    table['>'] = { 2, kn::funcs::greater };
    table['?'] = { 2, kn::funcs::equals };
    table['|'] = { 2, kn::funcs::disjunct };
    table['&'] = { 2, kn::funcs::conjunct };
    table[';'] = { 2, kn::funcs::sequence };
    table['='] = { 2, kn::funcs::assign };
    table['W'] = { 2, kn::funcs::while_ };

    table['I'] = { 3, kn::funcs::ifelse };
    table['G'] = { 3, kn::funcs::get };

    table['S'] = { 4, kn::funcs::substitute };

    return table;
  }();

}


namespace {

  struct NullExpr : kn::eval::Expression {
    kn::eval::Value evaluate() const override {
      return kn::eval::Null{};
    }
    void dump() const override {
      std::cout << "Null()";
    }
  };

  struct LitExpr : kn::eval::Expression {
    LitExpr(kn::lexer::StringLiteral s) : data(kn::eval::String(s.data)) {}
    LitExpr(kn::lexer::NumericLiteral n) : data(kn::eval::Number(n.data)) {}

    kn::eval::Value evaluate() const override {
      return data;
    }
    void dump() const override {
      std::cout << data;
    }
    kn::eval::Value data;
  };

}

namespace kn {

  eval::ExpressionPtr parse(const std::vector<kn::lexer::Token>& tokens) {
    if (tokens.empty())
      return std::make_unique<NullExpr>();

    // initialize the stack with a no-op
    std::vector<ParseInfo> stack;
    stack.push_back({ tokens.end(), 0, {}, 1, 0 });
    const auto top = [&stack]() -> decltype(auto) { return stack.back(); };

    // if we hit end while stack still has stuff:
    //   - we had an expression with missing arguments
    // if we run out of stack while tokens still have stuff:
    //   - we have random junk on the end
    // we finish iff (it == tokens.end() and expr_stack.size() == 1)
    auto it = tokens.begin();
    for (; it != tokens.end(); ++it) {
      if (auto x = it->as_string_lit()) {
        top().add_arg(std::make_unique<LitExpr>(*x));
      }
      else if (auto x = it->as_numeric_lit()) {
        top().add_arg(std::make_unique<LitExpr>(*x));
      }
      else if (auto x = it->as_function()) {
        auto f_id = static_cast<std::size_t>(x->id);
        if (auto [arity, fn] = functions[f_id]; fn) {
          if (arity == 0) {
            top().add_arg(fn({ it, 0, {}, 0, 0 }));
          } else {
            stack.push_back({ it, f_id, {}, arity, 0 });
          }
        } else {
          throw kn::Error(it->range(), "error: unknown function");
        }
      }
      else if (auto x = it->as_ident()) {
        top().add_arg(std::make_unique<eval::IdentExpr>(std::string(x->name)));
      }
      else {
        throw kn::Error(it->range(), "error: unknown token type");
      }

      // fold in completed stacks
      while (top().is_completed()) {
        if (stack.size() == 1) {
          // hit bottom of stack, we're done
          if (++it != tokens.end())
            throw kn::Error(it->pos(), "error: unparsed tokens");
          else
            return std::move(top().args[0]);
        } else {
          // pop the stack off, run the function, add to previous layer
          auto expr = functions[top().func_id].second(std::move(top()));
          stack.pop_back();
          top().add_arg(std::move(expr));
        }
      }
    }

    throw kn::Error(stack.back().token->pos(), "error: unexpected EOF");
  }

}
