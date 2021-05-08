#include <iterator>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <chrono>

#include "error.hpp"
#include "eval.hpp"
#include "funcs.hpp"
#include "ir.hpp"
#include "lexer.hpp"
#include "parser.hpp"

namespace {

  std::chrono::system_clock::time_point start;
  std::chrono::system_clock::time_point after_parsing;
  std::chrono::system_clock::time_point after_assembling;
  void on_exit() {
    using namespace std::chrono;
    using ms = duration<double, std::milli>;
    auto end = system_clock::now();
    std::cerr << "\n---\n\n";
    std::cerr << std::fixed << std::setprecision(4);
    std::cerr << "parse time:              " << std::setw(12)
      << ms(after_parsing - start).count() << "ms\n";
    std::cerr << "optimise/assemble time:  " << std::setw(12)
      << ms(after_assembling - after_parsing).count() << "ms\n";
    std::cerr << "execution time:          " << std::setw(12)
      << ms(end - after_assembling).count() << "ms\n";
    std::cerr << "total (excluding input): " << std::setw(12)
      << ms(end - start).count() << "ms\n";
  }

  void print_help_string(std::ostream& os, const char* program_name) {
    os
      << "usage: " << program_name
#ifdef KN_HAS_DEBUGGER
      << " [--debug]"
#endif
      << " [--time] [(-e <expr> | -f <filename>)]\n";
  }
}

int main([[maybe_unused]] int argc, char** argv) {
  using namespace std::literals;

  auto input = std::string{};

  auto supplied_input = false;
  auto timeit = false;
#ifdef KN_HAS_DEBUGGER
  auto should_run_debugger = false;
#endif

  for (char** curr_arg = argv + 1; *curr_arg != nullptr; ++curr_arg) {
    if (*curr_arg == "-f"sv and not supplied_input) {
      std::ifstream f(*++curr_arg);
      input.insert(input.end(), std::istreambuf_iterator<char>(f), {});
      supplied_input = true;
    } else if (*curr_arg == "-e"sv and not supplied_input) {
      input = *++curr_arg;
      supplied_input = true;
    } else if (*curr_arg == "--time"sv) {
      timeit = true;
#ifdef KN_HAS_DEBUGGER
    } else if (*curr_arg == "--debug"sv) {
      should_run_debugger = true;
#endif
    } else if (*curr_arg == "-h"sv or *curr_arg == "--help"sv) {
      print_help_string(std::cout, *argv);
      return 0;
    } else {
      std::cerr << "unknown argument \"" << *curr_arg << "\"\n";
      print_help_string(std::cerr, *argv);
      return 1;
    }
  }

  if (not supplied_input) {
    input.insert(input.end(), std::istreambuf_iterator<char>(std::cin), {});
  }

  if (input.empty()) {
    std::cerr << "no input\n";
    return 1;
  }

  try {
    start = std::chrono::system_clock::now();

    auto tokens = kn::lexer::tokenise(input);
    auto parsed = kn::parser::parse(tokens);
    after_parsing = std::chrono::system_clock::now();

    auto program = kn::ir::optimise(parsed);
    auto bytecode = kn::eval::prepare(program);
    after_assembling = std::chrono::system_clock::now();

    if (timeit) {
      if (std::atexit(on_exit) != 0)
        std::cerr << "warning: could not register timer function\n";
    }

#ifdef KN_HAS_DEBUGGER
    if (should_run_debugger)
      kn::eval::debug(std::move(bytecode));
    else
      kn::eval::run(std::move(bytecode));
#else
    kn::eval::run(std::move(bytecode));
#endif
  } catch (const kn::Error& err) {
    std::cout << err.what() << '\n';
    return 1;
  }
}

