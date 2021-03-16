#include "parser.hpp"

#include "error.hpp"
#include "funcs.hpp"
#include "lexer.hpp"
#include "value.hpp"

#include <string>
#include <array>
#include <limits>
#include <sstream>

using kn::lexer::TokenIter;

namespace {

  using parse_fn = kn::ExpressionPtr(const kn::ParseInfo&);
  using function_table =
    std::array<parse_fn*, std::numeric_limits<unsigned char>::max()>;

  auto functions = []() -> function_table {
    function_table table = { nullptr };

    table['T'] = kn::funcs::true_;
    table['F'] = kn::funcs::false_;
    table['N'] = kn::funcs::null;
    table['P'] = kn::funcs::prompt;
    table['R'] = kn::funcs::random;

    table['E'] = kn::funcs::eval;
    table['B'] = kn::funcs::block;
    table['C'] = kn::funcs::call;
    table['`'] = kn::funcs::shell;
    table['Q'] = kn::funcs::quit;
    table['!'] = kn::funcs::negate;
    table['L'] = kn::funcs::length;
    table['D'] = kn::funcs::dump;
    table['O'] = kn::funcs::output;

    table['+'] = kn::funcs::plus;
    table['-'] = kn::funcs::minus;
    table['*'] = kn::funcs::multiplies;
    table['/'] = kn::funcs::divides;
    table['%'] = kn::funcs::modulus;
    table['^'] = kn::funcs::exponent;
    table['<'] = kn::funcs::less;
    table['>'] = kn::funcs::greater;
    table['?'] = kn::funcs::identity;  // TODO: better name
    table['|'] = kn::funcs::disjunct;
    table['&'] = kn::funcs::conjunct;
    table[';'] = kn::funcs::sequence;
    table['='] = kn::funcs::assign;
    table['W'] = kn::funcs::while_;

    table['I'] = kn::funcs::ifelse;
    table['G'] = kn::funcs::get;

    table['S'] = kn::funcs::substitute;

    return table;
  }();

}


namespace {

  using namespace kn::eval;
  namespace lex = kn::lexer;

  [[noreturn]] void missing_argument(const kn::ParseInfo& info) {
    std::ostringstream oss;
    oss << "error: not enough arguments for: " << *info.curr_expr;
    throw kn::Error(info.curr_expr->range(), oss.str());
  }

  struct StringLitExpr : kn::Expression {
    StringLitExpr(lex::StringLiteral s) : data(String(s.data)) {}
    Value evaluate() const override {
      return data;
    }
    Value data;
  };

  struct NumericLitExpr : kn::Expression {
    NumericLitExpr(lex::NumericLiteral n) : data(Number(n.data)) {}
    Value evaluate() const override {
      return data;
    }
    Value data;
  };

  struct IdentifierExpr : kn::Expression {
    // TODO
  };

}

namespace kn {

  Expression::~Expression() = default;

  ExpressionPtr parse_impl(const ParseInfo& info) {
    auto& it = *info.iter;
    if (auto x = it->as_string_lit()) {
      return std::make_unique<StringLitExpr>(*x);
    }
    if (auto x = it->as_numeric_lit()) {
      return std::make_unique<NumericLitExpr>(*x);
    }
    if (auto x = it->as_function()) {
      if (auto f = functions[static_cast<unsigned char>(x->id)]) {
        return f(ParseInfo{ info.iter, *info.iter, info.end });
      } else {
        throw kn::Error(it->range(), "error: unknown function");
      }
    }
    std::ostringstream oss;
    oss << "error: unimplemented: " << *it;
    throw kn::Error(it->range(), oss.str());
  }

  ExpressionPtr parse(const std::vector<lex::Token>& tokens) {
    auto iter = tokens.begin();
    return parse_impl(ParseInfo{ &iter, iter, tokens.end() });
  }

  ExpressionPtr parse_arg(const ParseInfo& info) {
    if (*info.iter == info.end)
      missing_argument(info);
    ++*info.iter;
    return parse_impl(info);
  }

}
