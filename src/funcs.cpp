#include "funcs.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdlib>
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

  [[noreturn]] void unimplemented(const kn::ParseInfo& info) {
    throw kn::Error(info.token->range(), "error: unimplemented function");
  }

  template <std::size_t A, typename F>
  auto make_expression(kn::ParseInfo&& info, F f) {
    struct FunctionExpr : kn::Expression {
      FunctionExpr(F&& func, kn::ExpressionPtr* args) : func(std::move(func)) {
        std::copy(std::move_iterator(args), std::move_iterator(args + A),
                  children.begin());
      }

      kn::eval::Value evaluate() const override {
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
      std::array<kn::ExpressionPtr, A> children;
    };

    assert(info.arity == A);
    return std::make_unique<FunctionExpr>(std::move(f), info.args);
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

  ExpressionPtr block(ParseInfo info) { unimplemented(info); }
  ExpressionPtr call(ParseInfo info) { unimplemented(info); }
  ExpressionPtr shell(ParseInfo info) { unimplemented(info); }

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
      auto lhs_eval = lhs->evaluate();
      if (lhs_eval.is_number()) {
        auto lhs_val = lhs_eval.to_number();
        auto rhs_val = rhs->evaluate().to_number();
        return Value(lhs_val + rhs_val);
      } else if (lhs_eval.is_string()) {
        auto lhs_val = lhs_eval.to_string();
        auto rhs_val = rhs->evaluate().to_string();
        return Value(lhs_val + rhs_val);
      } else {
        // TODO: propagate location info and throw error
        assert(false);
      }
    });
  }

  ExpressionPtr minus(ParseInfo info) { unimplemented(info); }
  ExpressionPtr multiplies(ParseInfo info) { unimplemented(info); }
  ExpressionPtr divides(ParseInfo info) { unimplemented(info); }
  ExpressionPtr modulus(ParseInfo info) { unimplemented(info); }
  ExpressionPtr exponent(ParseInfo info) { unimplemented(info); }

  ExpressionPtr less(ParseInfo info) {
    return make_expression<2>(std::move(info), [](auto&& lhs, auto&& rhs) {
      auto lhs_eval = lhs->evaluate();
      if (lhs_eval.is_number()) {
        auto lhs_val = lhs_eval.to_number();
        auto rhs_val = rhs->evaluate().to_number();
        return lhs_val < rhs_val;
      } else if (lhs_eval.is_string()) {
        auto lhs_val = lhs_eval.to_string();
        auto rhs_val = rhs->evaluate().to_string();
        return lhs_val < rhs_val;
      } else if (lhs_eval.is_bool()) {
        auto lhs_val = lhs_eval.to_bool();
        auto rhs_val = rhs->evaluate().to_bool();
        return not lhs_val and rhs_val;
      } else {
        // TODO: propagate location info and throw error
        assert(false);
      }
    });
  }

  ExpressionPtr greater(ParseInfo info) { unimplemented(info); }

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
      if (auto id = dynamic_cast<IdentExpr*>(var.get())) {
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

  ExpressionPtr get(ParseInfo info) { unimplemented(info); }


  // arity 4


  ExpressionPtr substitute(ParseInfo info) { unimplemented(info); }

}
