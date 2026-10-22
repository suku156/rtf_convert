// =====================================================
// Module : ErrorHandle (implementation)
// =====================================================
#include"ErrorHandle.h"
#include"LogSystem_Module/LogSystem.h"
#include"ErrorSystem_Module/ErrorSystem.h"
#include"Universal_Module/ChangeStr.h"
#include<string>
#include<iostream>

void ErrorHandle::handleFatalGlobal(const std::wstring& message){
  terminate(message,EXIT_FAILURE);
}
void ErrorHandle::handle(const ErrorSystem::ErrorInfo& info){
  if(info.isEmpty()) return;
    if(!FileHeaderStarted_) return;
    
    if(logger_.isOpen()){
      logger_.log(info);
    }
    errorCount_ += info.getErrorCount();
    warringCount_ += info.getWarringCount();
    if(info.getMaxLevel() == ErrorSystem::ErrorLevel::FatalLocal ||
       info.getMaxLevel() == ErrorSystem::ErrorLevel::Recoverable)
    {
      hasFatalLocal_ = true;
    }
}
void ErrorHandle::beginFile(){
    errorCount_ = 0;
    warringCount_ = 0;
    FileHeaderStarted_ = true;
    hasFatalLocal_ = false;
}
void ErrorHandle::endFile() noexcept{
  if(!FileHeaderStarted_) return;

    // 對可能影響下一份資料的資料成員進行對應的措施
    errorCount_ = 0;
    warringCount_ = 0;
    FileHeaderStarted_ = false;
    hasFatalLocal_ = false;
}
[[noreturn]] void ErrorHandle::terminate(const std::wstring& message , int exitCode){
  #ifdef _WIN32
  std::wcerr <<L"[Fatal Error] "<< message << L"\n→ 程式中止 (Exit Code = " << exitCode << L")\n";
  #else
  std::cerr <<"[Fatal Error] "<< ChangeStr::wstringToUtf8(message) << "\n→ 程式中止 (Exit Code = " << exitCode << ")\n";
  #endif

  std::exit(exitCode);
}