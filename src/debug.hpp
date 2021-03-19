#ifndef KNIGHT_DEBUG_HPP_INCLUDED
#define KNIGHT_DEBUG_HPP_INCLUDED

#ifndef NDEBUG

#include <iostream>
#include <iomanip>

#include "env.hpp"
#include "eval.hpp"
#include "parser.hpp"

namespace kn::eval {

  inline std::ostream& operator<<(std::ostream& os, kn::eval::Label l) {
    switch (l.cat()) {
    case kn::eval::LabelCat::Unused:
      return os << "[?]";
    case kn::eval::LabelCat::Constant:
      return os << "$" << l.id();
    case kn::eval::LabelCat::Variable:
      return os << "[v:" << l.id() << "]";
    case kn::eval::LabelCat::Temporary:
      return os << "[t:" << l.id() << "]";
    case kn::eval::LabelCat::Literal:
      return os << Environment::get().value(l);
    case kn::eval::LabelCat::JumpTarget:
      return os << "[j:" << l.id() << "]";
    default:
      return os << "[!]";
    }
  }

  inline std::ostream& operator<<(std::ostream& os, OpCode op) {
    switch (op) {
    case kn::eval::OpCode::NoOp:
      os << "    "; break;
    case kn::eval::OpCode::Label:
      os << "#l  "; break;
    case kn::eval::OpCode::BlockData:
      os << "#b  "; break;
    case kn::eval::OpCode::Call:
      os << "cl  "; break;
    case kn::eval::OpCode::Return:
      os << "$   "; break;
    case kn::eval::OpCode::Jump:
      os << "j   "; break;
    case kn::eval::OpCode::JumpIf:
      os << "jy  "; break;
    case kn::eval::OpCode::JumpIfNot:
      os << "jn  "; break;

    // arithmetic
    case kn::eval::OpCode::Plus:
      os << "+   "; break;
    case kn::eval::OpCode::Minus:
      os << "-   "; break;
    case kn::eval::OpCode::Multiplies:
      os << "*   "; break;
    case kn::eval::OpCode::Divides:
      os << "/   "; break;
    case kn::eval::OpCode::Modulus:
      os << "%   "; break;
    case kn::eval::OpCode::Exponent:
      os << "^   "; break;

    // logical
    case kn::eval::OpCode::Negate:
      os << "!   "; break;
    case kn::eval::OpCode::Less:
      os << "<   "; break;
    case kn::eval::OpCode::Greater:
      os << ">   "; break;
    case kn::eval::OpCode::Equals:
      os << "?   "; break;

    // string
    case kn::eval::OpCode::Length:
      os << "len "; break;
    case kn::eval::OpCode::Get:
      os << "get "; break;
    case kn::eval::OpCode::Substitute:
      os << "sub "; break;

    // environment
    case kn::eval::OpCode::Assign:
      os << "=   "; break;
    case kn::eval::OpCode::Prompt:
      os << "inp "; break;
    case kn::eval::OpCode::Output:
      os << "out "; break;
    case kn::eval::OpCode::Random:
      os << "rnd "; break;
    case kn::eval::OpCode::Shell:
      os << "sh  "; break;
    case kn::eval::OpCode::Quit:
      os << "q   "; break;
    case kn::eval::OpCode::Eval:
      os << "ev  "; break;
    case kn::eval::OpCode::Dump:
      os << "dmp "; break;

    case kn::eval::OpCode::NumberOfOps:
      break;
    }

    return os;
  }

}

namespace kn::parser {

  inline std::ostream& operator<<(std::ostream& os, const Emitted& program) {
    for (auto&& op : program.instructions) {
      if (op.op == kn::eval::OpCode::Label) {
        os << op.labels[0] << " =>\n";
        continue;
      }
      os << "   " << op.op;

      for (std::size_t i = 0; i < kn::eval::max_labels; i++) {
        if (op.labels[i].cat() == kn::eval::LabelCat::Unused)
          break;
        os << op.labels[i] << ' ';
      }
      os << "\n";
    }
    return os;
  }

}

#endif // NDEBUG

#endif // KNIGHT_DEBUG_HPP_INCLUDED
