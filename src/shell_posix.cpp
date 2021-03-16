// this is pretty dodgy
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <memory>
#include <iostream>

#include "funcs.hpp"
#include "error.hpp"

std::string kn::funcs::open_shell(std::string command) {
  auto close = [](FILE* f) { pclose(f); };
  auto stream = std::unique_ptr<FILE, decltype(close)>(
    popen(command.c_str(), "r"), close);

  if (not stream)
    throw kn::Error("error: unable to execute command: " + command);

  std::string result;
  char buffer[4096];
  while (auto read = ::fread(buffer, 1, sizeof buffer, stream.get()))
    result.append(buffer, read);

  return result;
}
