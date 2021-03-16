#ifndef KNIGHT_SOURCEPOS_HPP_INCLUDED
#define KNIGHT_SOURCEPOS_HPP_INCLUDED

#include <cstddef>
#include <iterator>
#include <locale>
#include <string>

namespace kn {

  struct SourcePosition {
    int line;
    int column;

    std::string to_string() const {
      return std::to_string(line) + ":" + std::to_string(column);
    }

    friend constexpr bool operator==(SourcePosition lhs, SourcePosition rhs) noexcept {
      return lhs.line == rhs.line and lhs.column == rhs.column;
    }
    friend constexpr bool operator!=(SourcePosition lhs, SourcePosition rhs) noexcept {
      return !(lhs == rhs);
    }
  };

  // combine a SourcePosition on a character stream
  class SourceIterator {
  public:
    using value_type = char;
    using reference = const char&;
    using difference_type = std::ptrdiff_t;
    using pointer = void;
    using iterator_category = std::forward_iterator_tag;

    constexpr SourceIterator(const char* c, SourcePosition pos = { 1, 1 })
      : c(c), m_pos(pos)
    {}

    [[nodiscard]] constexpr const char* base() const noexcept { return c; }
    [[nodiscard]] constexpr SourcePosition pos() const noexcept { return m_pos; }

    friend constexpr
    bool operator==(const SourceIterator& lhs, const SourceIterator& rhs) noexcept {
      return lhs.c == rhs.c;
    }
    friend constexpr
    bool operator==(const SourceIterator& lhs, const char* rhs) noexcept {
      return lhs.c == rhs;
    }
    friend constexpr
    bool operator==(const char* lhs, const SourceIterator& rhs) noexcept {
      return lhs == rhs.c;
    }

    friend constexpr
    bool operator!=(const SourceIterator& lhs, const SourceIterator& rhs) noexcept {
      return !(lhs == rhs);
    }
    friend constexpr
    bool operator!=(const SourceIterator& lhs, const char* rhs) noexcept {
      return !(lhs == rhs);
    }
    friend constexpr
    bool operator!=(const char* lhs, const SourceIterator& rhs) noexcept {
      return !(lhs == rhs);
    }

    constexpr reference operator*() const noexcept { return *c; }

    constexpr SourceIterator& operator++() noexcept {
      if (*c == '\n') {
        ++m_pos.line;
        m_pos.column = 1;
      } else {
        ++m_pos.column;
      }
      ++c;
      return *this;
    }

    constexpr SourceIterator operator++(int) noexcept {
      SourceIterator old{ *this };
      ++*this;
      return old;
    }

  private:
    const char* c;
    SourcePosition m_pos;
  };

}

#endif // KNIGHT_SOURCEPOS_HPP_INCLUDED
