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

  using emit_fn = kn::parser::Emitted(*)(
    kn::parser::ASTFrame, kn::parser::ParseInfo&);
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

  std::vector<Block> parse(const std::vector<kn::lexer::Token>& tokens) {
    if (tokens.empty())
      return {};

    ParseInfo info;

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
      if (auto s = it->as_string_lit()) {
        top().add_child(env.get_string_literal(std::string(s->data)));
      }
      else if (auto n = it->as_numeric_lit()) {
        top().add_child(eval::Label::from_constant(n->data));
      }
      else if (auto i = it->as_ident()) {
        top().add_child(env.get_variable(std::string(i->name)));
      }
      else if (auto f = it->as_function()) {
        auto f_id = static_cast<std::size_t>(f->id);
        if (auto [arity, fn] = emitters[f_id]; fn) {
          if (arity == 0) {
            top().add_child(fn({ 0, {}, 0, 0 }, info));
          } else {
            stack.push_back({ f_id, {}, arity, 0 });
            // special case for blocks to track number of temporaries
            if (f_id == 'B') info.push_frame();
          }
        } else {
          throw kn::Error(it->range(), "error: unknown function");
        }
      }
      else {
        throw kn::Error(it->range(), "error: unknown token type");
      }

      // TODO: neaten?
      // fold in completed stacks
      while (top().is_completed()) {
        if (stack.size() == 1) {
          // hit bottom of stack, we're done
          if (++it != tokens.end()) {
            throw kn::Error(it->pos(), "error: unparsed tokens");
          } else {
            auto res = std::move(top().children[0]);
            res.instructions.emplace_back(eval::OpCode::Return, res.result);
            // prepend the number of temporaries we need
            res.instructions.emplace_front(
              eval::OpCode::BlockData, eval::Label::from_constant(info.pop_frame()));
            // add it to the list of blocks
            info.blocks.insert(info.blocks.begin(), std::move(res.instructions));
            return info.blocks;
          }
        } else {
          // pop the stack off, run the function, add to previous layer
          auto expr = emitters[top().func].second(std::move(top()), info);
          stack.pop_back();
          top().add_child(std::move(expr));
        }
      }
    }

    throw kn::Error("error: unexpected EOF");
  }

}
