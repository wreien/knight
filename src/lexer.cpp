#include "lexer.hpp"

#include <algorithm>
#include <cassert>
#include <charconv>
#include <iterator>

#include "sourcepos.hpp"
#include "error.hpp"

namespace {

  // character categorisation, assume ASCII

  constexpr bool is_numeric(char c) { return '0' <= c and c <= '9'; }

  constexpr bool is_head(char c) { return ('a' <= c and c <= 'z') or c == '_'; }
  constexpr bool is_ident(char c) { return is_head(c) or is_numeric(c); }

  constexpr bool is_func_head(char c) { return 'A' <= c and c <= 'Z'; }
  constexpr bool is_func_cont(char c) { return is_func_head(c) or c == '_'; }

  // types and helpers for parsing

  using iter = kn::SourceIterator;
  struct ParseResult {
    kn::lexer::Token token;
    kn::SourceIterator next_iter;
  };

  template <typename Pred>
  iter find_last_contiguous(iter first, iter end, Pred pred) {
    return std::adjacent_find(first, end, [pred](char lhs, char rhs) {
      return pred(lhs) != pred(rhs);
    });
  }

  // parse functions

  ParseResult parse_string_literal(iter first, iter end) {
    assert(*first == '\'' or *first == '"');

    // find next matching quote char
    if (auto last = std::find(std::next(first), end, *first); last != end) {
      auto next_iter = std::next(last);
      // contents of literal is within the quotes
      auto lit = std::string_view(
        first.base() + 1,
        static_cast<std::size_t>(next_iter.base() - first.base() - 2));

      // position of literal includes the quotes
      return ParseResult{
        { kn::lexer::StringLiteral{ lit }, first.pos(), last.pos() },
        next_iter
      };
    } else {
      throw kn::Error(first.pos(), "error: unterminated string literal");
    }
  }

  ParseResult parse_numeric_literal(iter first, iter end) {
    assert(is_numeric(*first));

    auto last = find_last_contiguous(first, end, is_numeric);
    auto next_iter = std::next(last);

    int result = 0;
    auto [_, ec] = std::from_chars(first.base(), next_iter.base(), result);
    if (ec != std::errc()) {
      if (ec == std::errc::result_out_of_range)
        throw kn::Error(first.pos(), last.pos(), "error: number out of range");
      else
        throw kn::Error(first.pos(), last.pos(), "error: couldn't parse literal");
    }

    return ParseResult{
      { kn::lexer::NumericLiteral{ result }, first.pos(), last.pos() },
      next_iter
    };
  }

  ParseResult parse_identifier(iter first, iter end) {
    assert(is_head(*first));

    auto last = find_last_contiguous(first, end, is_ident);
    auto next_iter = std::next(last);

    auto ident = std::string_view(
      first.base(), static_cast<std::size_t>(next_iter.base() - first.base()));
    return ParseResult{
      { kn::lexer::Identifier{ ident }, first.pos(), last.pos() },
      next_iter
    };
  }

  ParseResult parse_function(iter first, iter end) {
    if (is_func_head(*first)) {
      auto last = find_last_contiguous(first, end, is_func_cont);
      auto next_iter = std::next(last);
      return { { kn::lexer::Function{ *first }, first.pos(), last.pos() }, next_iter };
    } else {
      auto next_iter = std::next(first);
      return { { kn::lexer::Function{ *first }, first.pos() }, next_iter };
    }
  }

}

namespace kn::lexer {

  std::vector<Token> tokenise(std::string_view str) {
    auto results = std::vector<Token>{};

    // don't use .begin()/.end(), since not guaranteed be a char*
    auto it = SourceIterator(str.data());
    auto end = SourceIterator(str.data() + str.size());

    while (it != end) {
      switch (*it) {
      case '\t': case '\n': case '\r':
      case ' ': case ':':
      case '(': case ')':
      case '[': case ']':
      case '{': case '}': {
        // whitespace
        ++it;
        break;
      }

      case '#': {
        // comments
        it = std::find(it, end, '\n');
        ++it;
      } break;

      case '\'': case '"': {
        // string literals
        auto result = parse_string_literal(it, end);
        it = result.next_iter;
        results.push_back(std::move(result).token);
      } break;

      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9': {
        // numeric literals
        auto result = parse_numeric_literal(it, end);
        it = result.next_iter;
        results.push_back(std::move(result).token);
      } break;

      default:
        // everything else
        if (is_head(*it)) {
          auto result = parse_identifier(it, end);
          it = result.next_iter;
          results.push_back(std::move(result).token);
        } else {
          auto result = parse_function(it, end);
          it = result.next_iter;
          results.push_back(std::move(result).token);
        }
      }
    }

    return results;
  }

}
