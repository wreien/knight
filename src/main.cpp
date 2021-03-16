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
    auto expr = kn::parse(tokens);
    auto value = expr->evaluate();
    //std::cout << "result: " << value << '\n';
  } catch (const kn::Error& err) {
    std::cout << err.what() << '\n';
    return 1;
  }
}
