#pragma once
#include<cstddef>
#include<string>

// forward declaration
class logSystem;
namespace ErrorSystem{
    class ErrorInfo;
}

class ErrorHandle{
 bool FileHeaderStarted_ = false;
 bool hasFatalLocal_ = false;
 size_t errorCount_ = 0;
 size_t warringCount_ = 0;
 logSystem& logger_;
public:
  explicit ErrorHandle(logSystem& logger):logger_(logger){}
  void handleFatalGlobal(const std::wstring& message);
  void handle(const ErrorSystem::ErrorInfo& info);
  void beginFile();
  void endFile() noexcept;
private:
  [[noreturn]] void terminate(const std::wstring& message , int exitCode);
};

// 用於自動跑 ErrorHandle 的 endfile()
class FileScopeGuard{
  ErrorHandle& eh;
public:
  FileScopeGuard() = delete;
  FileScopeGuard(const FileScopeGuard&) = delete;
  FileScopeGuard& operator=(const FileScopeGuard&) = delete;

  explicit FileScopeGuard(ErrorHandle& handler) : eh(handler){}

  ~FileScopeGuard() noexcept{
    eh.endFile();
  }
};