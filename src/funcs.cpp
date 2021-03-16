#include "funcs.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdlib>
#include <cmath>
#include <functional>
#include <iostream>
#include <iterator>
#include <random>
#include <string>
#include <tuple>

#include "error.hpp"
#include "env.hpp"
#include "lexer.hpp"
#include "parser.hpp"


namespace {

  template <std::size_t A, typename F>
  auto make_expression(kn::ParseInfo&& info, F f) {
    using namespace kn::eval;

    struct FunctionExpr : Expression {
      FunctionExpr(F&& func, ExpressionPtr* args) : func(std::move(func)) {
        std::copy(std::move_iterator(args), std::move_iterator(args + A),
                  children.begin());
      }

      Value evaluate() const override {
        if constexpr (std::is_void_v<decltype(std::apply(func, children))>) {
          std::apply(func, children);
          return kn::eval::Null{};
        } else {
          return std::apply(func, children);
        }
      }

      void dump() const override {
        std::cout << "Function(" << A;
        for (auto&& c : children) {
          std::cout << " ";
          c->dump();
        }
        std::cout << ")";
      }

      F func;
      std::array<ExpressionPtr, A> children;
    };

    assert(info.arity == A);
    return std::make_unique<FunctionExpr>(std::move(f), info.args);
  }

  struct BlockExpr : kn::eval::Expression {
    BlockExpr(kn::eval::ExpressionPtr expr) : data(std::move(expr)) {}

    kn::eval::Value evaluate() const override {
      return data;
    }

    void dump() const override {
      std::cout << "Block(";
      data.expr->dump();
      std::cout << ")";
    }

    kn::eval::Block data;
  };

  template <typename F>
  auto make_binary_math_op(kn::ParseInfo&& info, F f) {
    return make_expression<2>(std::move(info), [f](auto&& lhs, auto&& rhs) {
      auto lhs_val = lhs->evaluate();
      if (lhs_val.is_number()) {
        auto x = lhs_val.to_number();
        auto y = rhs->evaluate().to_number();
        return f(x, y);
      } else {
        // TODO: propagate location info and throw error
        assert(false);
      }
    });
  }

  template <typename F>
  auto make_comparison_op(kn::ParseInfo&& info, F f) {
    return make_expression<2>(std::move(info), [f](auto&& lhs, auto&& rhs) {
      auto lhs_val = lhs->evaluate();
      if (lhs_val.is_number()) {
        auto x = lhs_val.to_number();
        auto y = rhs->evaluate().to_number();
        return f(x, y);
      } else if (lhs_val.is_string()) {
        auto x = lhs_val.to_string();
        auto y = rhs->evaluate().to_string();
        return f(x, y);
      } else if (lhs_val.is_bool()) {
        auto x = lhs_val.to_bool();
        auto y = rhs->evaluate().to_bool();
        return f(x, y);
      } else {
        // TODO: propagate location info and throw error
        assert(false);
      }
    });
  }

}

namespace kn::funcs {

  using namespace kn::eval;

  // arity 0

  ExpressionPtr true_(ParseInfo info) {
    return make_expression<0>(std::move(info), []{ return true; });
  }

  ExpressionPtr false_(ParseInfo info) {
    return make_expression<0>(std::move(info), []{ return false; });
  }

  ExpressionPtr null(ParseInfo info) {
    return make_expression<0>(std::move(info), []{ return Null{}; });
  }

  ExpressionPtr prompt(ParseInfo info) {
    return make_expression<0>(std::move(info), []{
      std::string line; std::getline(std::cin, line); return line; });
  }

  ExpressionPtr random(ParseInfo info) {
    thread_local auto generator = []{
      std::random_device rd;
      std::mt19937 gen(rd());
      return gen;
    }();
    thread_local auto dist = std::uniform_int_distribution(
      eval::Number{}, std::numeric_limits<eval::Number>::max());
    return make_expression<0>(std::move(info), []{ return dist(generator); });
  }


  // arity 1


  ExpressionPtr eval(ParseInfo info) {
    return make_expression<1>(std::move(info), [](auto&& expr) {
      auto str = expr->evaluate().to_string();
      auto tokens = kn::lexer::tokenise(str);
      return kn::parse(tokens)->evaluate();
    });
  }

  ExpressionPtr block(ParseInfo info) {
    assert(info.arity == 1);
    return std::make_unique<BlockExpr>(std::move(info.args[0]));
  }

  ExpressionPtr call(ParseInfo info) {
    return make_expression<1>(std::move(info), [](auto&& expr) {
      auto block = expr->evaluate().to_block();
      return block.expr->evaluate();
    });
  }

  ExpressionPtr shell(ParseInfo info) {
    return make_expression<1>(std::move(info), [](auto&& expr) {
      return open_shell(expr->evaluate().to_string());
    });
  }

  ExpressionPtr quit(ParseInfo info) {
    return make_expression<1>(std::move(info), [](auto&& expr) {
      std::exit(expr->evaluate().to_number());
    });
  }

  ExpressionPtr negate(ParseInfo info) {
    return make_expression<1>(std::move(info), [](auto&& expr) {
      return not expr->evaluate().to_bool();
    });
  }

  ExpressionPtr length(ParseInfo info) {
    return make_expression<1>(std::move(info), [](auto&& expr) {
      return static_cast<Number>(expr->evaluate().to_string().length());
    });
  }

  ExpressionPtr dump(ParseInfo info) {
    return make_expression<1>(std::move(info), [](auto&& expr) {
      expr->dump(); return Null{};
    });
  }

  ExpressionPtr output(ParseInfo info) {
    return make_expression<1>(std::move(info), [](auto&& expr) {
      // by spec always flush output
      auto str = expr->evaluate().to_string();
      if (not str.empty() and str.back() == '\\') {
        str.pop_back();
        std::cout << str << std::flush;
      } else {
        std::cout << str << '\n' << std::flush;
      }
    });
  }


  // arity 2


  ExpressionPtr plus(ParseInfo info) {
    return make_expression<2>(std::move(info), [](auto&& lhs, auto&& rhs) {
      auto lhs_val = lhs->evaluate();
      if (lhs_val.is_number()) {
        auto x = lhs_val.to_number();
        auto y = rhs->evaluate().to_number();
        return Value(x + y);
      } else if (lhs_val.is_string()) {
        auto x = lhs_val.to_string();
        auto y = rhs->evaluate().to_string();
        return Value(x + y);
      } else {
        // TODO: propagate location info and throw error
        assert(false);
      }
    });
  }

  ExpressionPtr multiplies(ParseInfo info) {
    return make_expression<2>(std::move(info), [](auto&& lhs, auto&& rhs) {
      auto lhs_val = lhs->evaluate();
      if (lhs_val.is_number()) {
        auto x = lhs_val.to_number();
        auto y = rhs->evaluate().to_number();
        return Value(x * y);
      } else if (lhs_val.is_string()) {
        auto x = lhs_val.to_string();
        auto y = static_cast<std::size_t>(rhs->evaluate().to_number());
        std::string s;
        s.reserve(x.size() * y);
        while (y--) s += x;
        return Value(std::move(s));
      } else {
        // TODO: propagate location info and throw error
        assert(false);
      }
    });
  }

  ExpressionPtr minus(ParseInfo info) {
    return make_binary_math_op(std::move(info), std::minus{});
  }

  ExpressionPtr divides(ParseInfo info) {
    return make_binary_math_op(std::move(info), std::divides{});
  }

  ExpressionPtr modulus(ParseInfo info) {
    // TODO: doesn't line up with spec; also spec is self-contradictory
    return make_binary_math_op(std::move(info), std::modulus{});
  }

  ExpressionPtr exponent(ParseInfo info) {
    return make_binary_math_op(std::move(info), [](Number lhs, Number rhs) {
      return static_cast<Number>(std::pow(lhs, rhs)); });
  }

  ExpressionPtr less(ParseInfo info) {
    return make_comparison_op(std::move(info), std::less{});
  }

  ExpressionPtr greater(ParseInfo info) {
    return make_comparison_op(std::move(info), std::greater{});
  }

  ExpressionPtr equals(ParseInfo info) {
    return make_expression<2>(std::move(info), [](auto&& lhs, auto&& rhs) {
      auto lhs_val = lhs->evaluate();
      auto rhs_val = rhs->evaluate();
      return lhs_val == rhs_val;
    });
  }

  ExpressionPtr conjunct(ParseInfo info) {
    return make_expression<2>(std::move(info), [](auto&& lhs, auto&& rhs) {
      auto lhs_val = lhs->evaluate();
      if (not lhs_val.to_bool())
        return lhs_val;
      return rhs->evaluate();
    });
  }

  ExpressionPtr disjunct(ParseInfo info) {
    return make_expression<2>(std::move(info), [](auto&& lhs, auto&& rhs) {
      auto lhs_val = lhs->evaluate();
      if (lhs_val.to_bool())
        return lhs_val;
      return rhs->evaluate();
    });
  }

  ExpressionPtr sequence(ParseInfo info) {
    return make_expression<2>(std::move(info), [](auto&& lhs, auto&& rhs) {
      // ignore return value
      lhs->evaluate();
      return rhs->evaluate();
    });
  }

  ExpressionPtr assign(ParseInfo info) {
    return make_expression<2>(std::move(info), [](auto&& var, auto&& expr) {
      if (auto id = dynamic_cast<const IdentExpr*>(var.get())) {
        return Environment::get().assign(id->data, expr->evaluate());
      } else {
        // TODO: propagate location info and throw
        assert(false);
      }
    });
  }

  ExpressionPtr while_(ParseInfo info) {
    return make_expression<2>(std::move(info), [](auto&& cond, auto&& block) {
      while (cond->evaluate().to_bool())
        block->evaluate();
      return Null{};
    });
  }


  // arity 3


  ExpressionPtr ifelse(ParseInfo info) {
    return make_expression<3>(std::move(info), [](auto&& cond, auto&& t, auto&& f) {
      if (cond->evaluate().to_bool()) {
        return t->evaluate();
      } else {
        return f->evaluate();
      }
    });
  }

  ExpressionPtr get(ParseInfo info) {
    return make_expression<3>(std::move(info), [](auto&& s, auto&& i, auto&& c) {
      auto str = s->evaluate().to_string();
      auto idx = static_cast<std::size_t>(i->evaluate().to_number());
      auto cnt = static_cast<std::size_t>(c->evaluate().to_number());
      return str.substr(idx, cnt);
    });
  }


  // arity 4


  ExpressionPtr substitute(ParseInfo info) {
    return make_expression<4>(std::move(info),
      [](auto&& s, auto&& i, auto&& c, auto&& r) {
        auto str = s->evaluate().to_string();
        auto idx = static_cast<std::size_t>(i->evaluate().to_number());
        auto cnt = static_cast<std::size_t>(c->evaluate().to_number());
        auto replace = r->evaluate().to_string();
        return str.replace(idx, cnt, replace);
      });
  }

}
