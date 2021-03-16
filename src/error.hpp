#ifndef KNIGHT_ERROR_HPP_INCLUDED
#define KNIGHT_ERROR_HPP_INCLUDED

#include <exception>
#include <utility>
#include <string_view>
#include <string>
#include "sourcepos.hpp"

namespace kn {

  struct Error : std::exception {
    Error(SourcePosition first, SourcePosition last, std::string_view msg)
      : m_msg((first.to_string() + ": ").append(msg)), m_first(first), m_last(last)
    {}
    Error(SourcePosition pos, std::string_view msg)
      : Error(pos, pos, msg)
    {}
    Error(std::pair<SourcePosition, SourcePosition> range, std::string_view msg)
      : Error(range.first, range.second, msg)
    {}

    SourcePosition pos() const noexcept {
      return m_first;
    }

    std::pair<SourcePosition, SourcePosition> range() const noexcept {
      return { m_first, m_last };
    }

    const char* what() const noexcept override {
      return m_msg.c_str();
    }

  private:
    std::string m_msg;
    SourcePosition m_first;
    SourcePosition m_last;
  };

}


#endif  // KNIGHT_ERROR_HPP_INCLUDED
