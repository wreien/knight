#include <Windows.h>
#include <memory>
#include <string>
#include <iterator>

#include "funcs.hpp"
#include "error.hpp"

namespace {

  struct HandleDeleter {
    using pointer = HANDLE;
    void operator()(HANDLE h) const noexcept {
      CloseHandle(h);
    }
  };
  using RaiiHandle = std::unique_ptr<HANDLE, HandleDeleter>;

  std::wstring to_utf16(const std::string& str) {
    if (str.empty()) return {};
    constexpr auto flags = MB_ERR_INVALID_CHARS;

    auto length = MultiByteToWideChar(
      CP_ACP,
      flags,
      str.data(),
      static_cast<int>(str.size()),
      nullptr,
      0);
    if (length == 0)
      throw kn::Error("error: to_utf16: prepare MultiByteToWideChar() failed");

    std::wstring utf16(static_cast<std::wstring::size_type>(length), L'\0');
    auto result = MultiByteToWideChar(
      CP_ACP,
      flags,
      str.data(),
      static_cast<int>(str.size()),
      utf16.data(),
      static_cast<int>(utf16.size()));
    if (result == 0)
      throw kn::Error("error: to_utf16: exec MultiByteToWideChar() failed");

    return utf16;
  }

}

std::string kn::funcs::open_shell(const std::string& command) {
  using namespace std::literals;

  auto attrs = SECURITY_ATTRIBUTES{};
  attrs.nLength = sizeof attrs;
  attrs.bInheritHandle = TRUE;
  attrs.lpSecurityDescriptor = nullptr;

  // create an anonymous pipe for the process's stdout
  auto stdout_read = RaiiHandle{};
  auto stdout_write = RaiiHandle{};
  if (HANDLE r, w; CreatePipe(&r, &w, &attrs, 0)) {
    stdout_read.reset(r);
    stdout_write.reset(w);
  } else
    throw kn::Error("error: open_shell: CreatePipe() failed");
  
  // ensure our end of the pipe doesn't get inherited
  if (not SetHandleInformation(stdout_read.get(), HANDLE_FLAG_INHERIT, 0))
    throw kn::Error("error: open_shell: SetHandleInformation() failed");

  // we delegate to powershell to do the stuff;
  // also, transform to UTF16 to satisfy windows
  std::wstring translated = to_utf16("powershell.exe -Command " + command);
  
  // create the process
  auto startup_info = STARTUPINFOW{};
  startup_info.cb = sizeof startup_info;
  startup_info.dwFlags = STARTF_USESTDHANDLES | STARTF_UNTRUSTEDSOURCE;
  startup_info.hStdOutput = stdout_write.get();
  auto proc_info = PROCESS_INFORMATION{};
  auto succeeded = CreateProcessW(
    nullptr, 
    translated.data(),
    nullptr,
    nullptr,
    TRUE,
    0,
    nullptr,
    nullptr,
    &startup_info,
    &proc_info);
  if (not succeeded)
    throw kn::Error("error: open_shell: CreateProcessW() failed");

  // close handles we don't care about anymore
  CloseHandle(proc_info.hProcess);
  CloseHandle(proc_info.hThread);
  stdout_write.reset();

  // read from our end of the pipe until it closes
  std::string result;
  char buffer[4096]{};
  auto read = DWORD{};
  while (ReadFile(stdout_read.get(), buffer, sizeof buffer, &read, nullptr)
         and read != 0) {
    // convert \r\n into \n
    for (auto it = buffer; it != buffer + read; ++it) {
      if (*it == '\r' and *std::next(it) == '\n')
        ++it;
      result.push_back(*it);
    }
  }

  return result;
}
