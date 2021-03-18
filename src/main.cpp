#include <iterator>
#include <iostream>
#include <fstream>
#include <string>

#include "error.hpp"
#include "eval.hpp"
#include "funcs.hpp"
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
    auto program = kn::parser::parse(tokens);

    debug(program);

    auto value = kn::eval::run(program);
    //std::cout << "result: " << value << '\n';
  } catch (const kn::Error& err) {
    std::cout << err.what() << '\n';
    return 1;
  }
}

std::string stringify(kn::eval::Label l) {
  switch (l.cat()) {
  case kn::eval::LabelCat::Unused: return "[?]";
  case kn::eval::LabelCat::Constant: return "[c:" + std::to_string(l.id()) + "]";
  case kn::eval::LabelCat::Variable: return "[v:" + std::to_string(l.id()) + "]";
  case kn::eval::LabelCat::JumpTarget: return "[l:" + std::to_string(l.id()) + "]";
  }
  return "[!]";
}

void debug(const kn::parser::Emitted& program) {
  for (auto&& op : program.instructions) {
    if (op.op == kn::eval::OpCode::Label) {
      std::cout << stringify(op.labels[0]) << " =>\n";
      continue;
    }
    std::cout << "   ";

    switch (op.op) {
    case kn::eval::OpCode::NoOp:
    case kn::eval::OpCode::Label:
    case kn::eval::OpCode::NumberOfOps:
      break;
    case kn::eval::OpCode::Call:
      std::cout << "cl  "; break;
    case kn::eval::OpCode::Return:
      std::cout << "$   "; break;
    case kn::eval::OpCode::Jump:
      std::cout << "j   "; break;
    case kn::eval::OpCode::JumpIf:
      std::cout << "jy  "; break;
    case kn::eval::OpCode::JumpIfNot:
      std::cout << "jn  "; break;

    // arithmetic
    case kn::eval::OpCode::Plus:
      std::cout << "+   "; break;
    case kn::eval::OpCode::Minus:
      std::cout << "-   "; break;
    case kn::eval::OpCode::Multiplies:
      std::cout << "*   "; break;
    case kn::eval::OpCode::Divides:
      std::cout << "/   "; break;
    case kn::eval::OpCode::Modulus:
      std::cout << "%   "; break;
    case kn::eval::OpCode::Exponent:
      std::cout << "^   "; break;

    // logical
    case kn::eval::OpCode::Negate:
      std::cout << "!   "; break;
    case kn::eval::OpCode::Less:
      std::cout << "<   "; break;
    case kn::eval::OpCode::Greater:
      std::cout << ">   "; break;
    case kn::eval::OpCode::Equals:
      std::cout << "?   "; break;

    // string
    case kn::eval::OpCode::Length:
      std::cout << "len "; break;
    case kn::eval::OpCode::Get:
      std::cout << "get "; break;
    case kn::eval::OpCode::Substitute:
      std::cout << "sub "; break;

    // environment
    case kn::eval::OpCode::Assign:
      std::cout << "=   "; break;
    case kn::eval::OpCode::Prompt:
      std::cout << "inp "; break;
    case kn::eval::OpCode::Output:
      std::cout << "out "; break;
    case kn::eval::OpCode::Random:
      std::cout << "rnd "; break;
    case kn::eval::OpCode::Shell:
      std::cout << "sh  "; break;
    case kn::eval::OpCode::Quit:
      std::cout << "q   "; break;
    case kn::eval::OpCode::Eval:
      std::cout << "ev  "; break;
    case kn::eval::OpCode::Dump:
      std::cout << "dmp "; break;
    }

    for (std::size_t i = 0; i < kn::eval::max_labels; i++) {
      if (op.labels[i].cat() == kn::eval::LabelCat::Unused)
        break;
      std::cout << stringify(op.labels[i]);
    }
    std::cout << "\n";
  }
}
