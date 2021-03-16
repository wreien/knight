#include <iterator>
#include <iostream>
#include <fstream>
#include <string>
#include "lexer.hpp"
#include "parser.hpp"
#include "funcs.hpp"
#include "error.hpp"

int main(int argc, char** argv) {
  using namespace std::literals;

  std::string input;
  if (argc >= 2 and argv[1] != "-"sv) {
    std::ifstream f(argv[1]);
    input.insert(input.end(), std::istreambuf_iterator<char>(f), {});
  } else {
    input.insert(input.end(), std::istreambuf_iterator<char>(std::cin), {});
  }

  try {
    auto tokens = kn::lexer::tokenise(input);
    auto expr = kn::parse(tokens);
    auto value = expr->evaluate();
    std::cout << "result: " << value << '\n';
  } catch (const kn::Error& err) {
    std::cout << err.what() << '\n';
  }
}
