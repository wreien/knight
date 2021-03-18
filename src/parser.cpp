#include "parser.hpp"

#include "emit.hpp"
#include "env.hpp"
#include "error.hpp"
#include "lexer.hpp"
#include "value.hpp"

#include <array>
#include <ostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace {

  using emit_fn = kn::parser::Emitted(*)(kn::parser::ASTFrame);
  using emitter_map = std::array<
    std::pair<int, emit_fn>,
    std::numeric_limits<unsigned char>::max()>;

  auto emitters = []() -> emitter_map {
    auto table = emitter_map{};

    table['T'] = { 0, kn::parser::emit::true_ };
    table['F'] = { 0, kn::parser::emit::false_ };
    table['N'] = { 0, kn::parser::emit::null };
    table['P'] = { 0, kn::parser::emit::prompt };
    table['R'] = { 0, kn::parser::emit::random };

    table['E'] = { 1, kn::parser::emit::eval };
    table['B'] = { 1, kn::parser::emit::block };
    table['C'] = { 1, kn::parser::emit::call };
    table['`'] = { 1, kn::parser::emit::shell };
    table['Q'] = { 1, kn::parser::emit::quit };
    table['!'] = { 1, kn::parser::emit::negate };
    table['L'] = { 1, kn::parser::emit::length };
    table['D'] = { 1, kn::parser::emit::dump };
    table['O'] = { 1, kn::parser::emit::output };

    table['+'] = { 2, kn::parser::emit::plus };
    table['-'] = { 2, kn::parser::emit::minus };
    table['*'] = { 2, kn::parser::emit::multiplies };
    table['/'] = { 2, kn::parser::emit::divides };
    table['%'] = { 2, kn::parser::emit::modulus };
    table['^'] = { 2, kn::parser::emit::exponent };
    table['<'] = { 2, kn::parser::emit::less };
    table['>'] = { 2, kn::parser::emit::greater };
    table['?'] = { 2, kn::parser::emit::equals };
    table['|'] = { 2, kn::parser::emit::disjunct };
    table['&'] = { 2, kn::parser::emit::conjunct };
    table[';'] = { 2, kn::parser::emit::sequence };
    table['='] = { 2, kn::parser::emit::assign };
    table['W'] = { 2, kn::parser::emit::while_ };

    table['I'] = { 3, kn::parser::emit::ifelse };
    table['G'] = { 3, kn::parser::emit::get };

    table['S'] = { 4, kn::parser::emit::substitute };

    return table;
  }();

}

namespace kn::parser {

  Emitted parse(const std::vector<kn::lexer::Token>& tokens) {
    if (tokens.empty())
      return {};

    // initialize the stack with a no-op
    std::vector<ASTFrame> stack;
    stack.push_back({ 0, {}, 1, 0 });

    // some helper functions
    const auto top = [&stack]() -> decltype(auto) {
      return stack.back();
    };
    auto& env = eval::Environment::get();

    // if we hit end while stack still has stuff:
    //   - we had an expression with missing arguments
    // if we run out of stack while tokens still have stuff:
    //   - we have random junk on the end
    // we finish iff (it == tokens.end() and expr_stack.size() == 1)
    auto it = tokens.begin();
    for (; it != tokens.end(); ++it) {
      if (auto x = it->as_string_lit()) {
        top().add_child(env.get_literal(eval::String(x->data)));
      }
      else if (auto x = it->as_numeric_lit()) {
        top().add_child(eval::Label::from_constant(x->data));
      }
      else if (auto x = it->as_ident()) {
        top().add_child(env.get_variable(eval::String(x->name)));
      }
      else if (auto x = it->as_function()) {
        auto f_id = static_cast<std::size_t>(x->id);
        if (auto [arity, fn] = emitters[f_id]; fn) {
          if (arity == 0) {
            top().add_child(fn({ 0, {}, 0, 0 }));
          } else {
            stack.push_back({ f_id, {}, arity, 0 });
          }
        } else {
          throw kn::Error(it->range(), "error: unknown function");
        }
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
            return top().children[0];
        } else {
          // pop the stack off, run the function, add to previous layer
          auto expr = emitters[top().func].second(std::move(top()));
          stack.pop_back();
          top().add_child(std::move(expr));
        }
      }
    }

    throw kn::Error("error: unexpected EOF");
  }

}
