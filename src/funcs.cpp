#include "funcs.hpp"

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <random>
#include <string>
#include <tuple>

#include "error.hpp"


namespace {

  [[noreturn]] void unimplemented(const kn::ParseInfo& info) {
    throw kn::Error((*info.iter)->range(), "error: unimplemented");
  }

  template <typename F, typename... Ts>
  auto make_expression(F f, Ts... children) {
    struct FunctionExpr : kn::Expression {
      FunctionExpr(F&& func, Ts&&... children)
        : func(std::move(func)), children(std::move(children)...)
      {}

      kn::eval::Value evaluate() const {
        if constexpr (std::is_void_v<decltype(std::apply(func, children))>) {
          std::apply(func, children);
          return kn::eval::Null{};
        } else {
          return std::apply(func, children);
        }
      }

      F func;
      std::tuple<Ts...> children;
    };

    return std::make_unique<FunctionExpr>(std::move(f), std::move(children)...);
  }
}

namespace kn::funcs {

  // arity 0

  ExpressionPtr true_(const ParseInfo&) {
    return make_expression([]{ return true; });
  }

  ExpressionPtr false_(const ParseInfo&) {
    return make_expression([]{ return false; });
  }

  ExpressionPtr null(const ParseInfo&) {
    return make_expression([]{ return kn::eval::Null{}; });
  }

  ExpressionPtr prompt(const ParseInfo&) {
    return make_expression([]{
      std::string line; std::getline(std::cin, line); return line; });
  }

  ExpressionPtr random(const ParseInfo&) {
    thread_local auto generator = []{
      std::random_device rd;
      std::mt19937 gen(rd());
      return gen;
    }();
    thread_local auto dist = std::uniform_int_distribution(
      eval::Number{}, std::numeric_limits<eval::Number>::max());
    return make_expression([]{ return dist(generator); });
  }


  // arity 1


  ExpressionPtr eval(const ParseInfo& info) { unimplemented(info); }
  ExpressionPtr block(const ParseInfo& info) { unimplemented(info); }
  ExpressionPtr call(const ParseInfo& info) { unimplemented(info); }
  ExpressionPtr shell(const ParseInfo& info) { unimplemented(info); }

  ExpressionPtr quit(const ParseInfo& info) {
    auto status_expr = kn::parse_arg(info);
    return make_expression([](const auto& status_expr) {
      std::exit(status_expr->evaluate().to_number());
    }, std::move(status_expr));
  }

  ExpressionPtr negate(const ParseInfo& info) { unimplemented(info); }
  ExpressionPtr length(const ParseInfo& info) { unimplemented(info); }
  ExpressionPtr dump(const ParseInfo& info) { unimplemented(info); }

  ExpressionPtr output(const ParseInfo& info) {
    auto str_expr = kn::parse_arg(info);
    return make_expression([](const auto& str_expr) {
      // by spec always flush output
      auto str = str_expr->evaluate().to_string();
      if (not str.empty() and str.back() == '\\') {
        str.pop_back();
        std::cout << str << std::flush;
      } else {
        std::cout << str << '\n' << std::flush;
      }
    }, std::move(str_expr));
  }


  // arity 2


  ExpressionPtr plus(const ParseInfo& info) {
    auto lhs = kn::parse_arg(info);
    auto rhs = kn::parse_arg(info);
    return make_expression([](const auto& lhs, const auto& rhs) -> eval::Value {
      auto lhs_eval = lhs->evaluate();
      if (lhs_eval.is_number()) {
        auto lhs_val = lhs_eval.to_number();
        auto rhs_val = rhs->evaluate().to_number();
        return lhs_val + rhs_val;
      } else if (lhs_eval.is_string()) {
        auto lhs_val = lhs_eval.to_string();
        auto rhs_val = rhs->evaluate().to_string();
        return lhs_val + rhs_val;
      } else {
        // TODO: propagate location info and throw error
        assert(false);
      }
    }, std::move(lhs), std::move(rhs));
  }

  ExpressionPtr minus(const ParseInfo& info) { unimplemented(info); }
  ExpressionPtr multiplies(const ParseInfo& info) { unimplemented(info); }
  ExpressionPtr divides(const ParseInfo& info) { unimplemented(info); }
  ExpressionPtr modulus(const ParseInfo& info) { unimplemented(info); }
  ExpressionPtr exponent(const ParseInfo& info) { unimplemented(info); }
  ExpressionPtr less(const ParseInfo& info) { unimplemented(info); }
  ExpressionPtr greater(const ParseInfo& info) { unimplemented(info); }
  ExpressionPtr identity(const ParseInfo& info) { unimplemented(info); }
  ExpressionPtr conjunct(const ParseInfo& info) { unimplemented(info); }
  ExpressionPtr disjunct(const ParseInfo& info) { unimplemented(info); }
  ExpressionPtr sequence(const ParseInfo& info) { unimplemented(info); }
  ExpressionPtr assign(const ParseInfo& info) { unimplemented(info); }
  ExpressionPtr while_(const ParseInfo& info) { unimplemented(info); }


  // arity 3


  ExpressionPtr ifelse(const ParseInfo& info) { unimplemented(info); }
  ExpressionPtr get(const ParseInfo& info) { unimplemented(info); }

  // arity 4

  ExpressionPtr substitute(const ParseInfo& info) { unimplemented(info); }

}
