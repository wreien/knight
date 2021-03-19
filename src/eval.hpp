#ifndef KNIGHT_EVAL_HPP_INCLUDED
#define KNIGHT_EVAL_HPP_INCLUDED

#include <cassert>
#include <cstddef>
#include <vector>

namespace kn::parser {

  struct Emitted;

}

namespace kn::eval {

  struct Value;

  enum class LabelCat {
    // not a valid label, used for sanity checking
    Unused,
    // immutable: a number, e.g. 100
    Constant,
    // mutable: a variable, can be assigned to
    Variable,
    // replaced: references a label to be jumped to
    JumpTarget,
    // immutable: a fresh temporary
    Temporary,
    // immutable: a literal
    Literal,
  };

  class Label {
  public:
    Label()
      : m_cat(LabelCat::Unused), m_id(0)
    {}

    Label(LabelCat cat, std::size_t id)
      : m_cat(cat), m_id(id)
    {}

    static Label from_constant(int n) {
      // numeric constants are always positive
      assert(n >= 0);
      return Label(LabelCat::Constant, static_cast<std::size_t>(n));
    }

    static Label from_constant(std::size_t n) {
      return Label(LabelCat::Constant, n);
    }

    LabelCat cat() const noexcept { 
      return m_cat; 
    }

    std::size_t id() const noexcept { 
      return m_id; 
    }

    // can the value change
    bool is_mutable() const noexcept { 
      return cat() != LabelCat::Variable; 
    }

    // do we need to dereference the label to get a result
    bool needs_eval() const noexcept {
      return cat() == LabelCat::Variable
          or cat() == LabelCat::Literal
          or cat() == LabelCat::Temporary;
    }

    friend bool operator==(Label a, Label b) noexcept {
      return a.m_cat == b.m_cat and a.m_id == b.m_id;
    }

  private:
    // TODO: bitpack?
    LabelCat m_cat;
    std::size_t m_id;
  };

  enum class OpCode {
    NoOp,

    // control flow
    Label,
    BlockData,
    Call,
    Return,
    Jump,
    JumpIf,
    JumpIfNot,

    // arithmetic
    Plus,
    Minus,
    Multiplies,
    Divides,
    Modulus,
    Exponent,

    // logical
    Negate,
    Less,
    Greater,
    Equals,

    // string
    Length,
    Get,
    Substitute,

    // environment
    Assign,
    Prompt,
    Output,
    Random,
    Shell,
    Quit,
    Eval,
    Dump,

    // total number of elements
    NumberOfOps
  };

  inline constexpr std::size_t max_labels = 5;

  struct Operation {
    template <typename... Args>
    Operation(OpCode op, Args... labels)
      : op(op), labels{ labels... }
    {}

    OpCode op;
    Label labels[max_labels];
  };

  // flattened representation of an operation
  union CodePoint {
    explicit CodePoint(OpCode op) : op(op) {}
    explicit CodePoint(Label label) : label(label) {}

    OpCode op;
    Label label;
  };
  using ByteCode = std::vector<CodePoint>;

  // prepare a program for execution
  // `offset` specifies how much to offset new addresses in the resultant code
  ByteCode prepare(const parser::Emitted& program, std::size_t offset = 0);

  // run a prepared program
  void run(ByteCode program);

#ifndef NDEBUG
  // step through a prepared program
  void debug(ByteCode program);
#endif

}

#endif // KNIGHT_EVAL_HPP_INCLUDED
