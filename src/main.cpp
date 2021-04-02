#include <iterator>
#include <iostream>
#include <fstream>
#include <string>

#include "debug.hpp"
#include "error.hpp"
#include "eval.hpp"
#include "funcs.hpp"
#include "ir.hpp"
#include "lexer.hpp"
#include "parser.hpp"

void debug(const kn::parser::Emitted&);

int main(int argc, char** argv) {
  using namespace std::literals;

  std::string input;
  if (argc == 2 or (argc == 3 and argv[1] == "-f"sv)) {
    std::ifstream f(argv[argc - 1]);
    input.insert(input.end(), std::istreambuf_iterator<char>(f), {});
  } else if (argc == 3 and argv[1] == "-e"sv) {
    input = argv[2];
  } else if (argc == 1) {
    input.insert(input.end(), std::istreambuf_iterator<char>(std::cin), {});
  } else {
    std::cout << "bad usage: ./knight [[(-e | -f)] input]\n";
  }

  try {
    auto tokens = kn::lexer::tokenise(input);
    auto parsed = kn::parser::parse(tokens);
    auto program = kn::ir::optimise(parsed);

#ifndef NDEBUG
    std::cout << parsed;
    std::cout << "\n===\n\n\n";
#endif
    auto bytecode = kn::eval::prepare(program);

#ifdef NDEBUG
    kn::eval::run(std::move(bytecode));
#else
    kn::eval::debug(std::move(bytecode));
#endif
  } catch (const kn::Error& err) {
    std::cout << err.what() << '\n';
    return 1;
  }
}

