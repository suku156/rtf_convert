// =====================================================
// Module  : LogSystem
// Author  : suku156
// Purpose : 用來記錄程式的動作
// Layer   : log
//
// Depend  :
//   ErrorSystem 
//
// Used by :
//   RTFProcessor   
//   ErrorHandle
//   PictProcessor
//   textProcessor
//
// Notes :
//   用來記錄程式的行動
//   正常執行或錯誤行為都會記錄 
// =====================================================
#pragma once
#include<fstream>
#include<mutex>
#include<filesystem>
#include<string>



// forward declaration 前向宣告 條件允許的話可以省去 include
namespace ErrorSystem{
   class ErrorInfo;
   enum class ErrorLevel : int; 
}

// 用來表示輸入資訊的類別
enum class LogLevel{
  Info,
  Warn,
  Error,
  Debug
};

// log 類別的宣告
class logSystem{
  std::ofstream output_;
  std::mutex mutex_; // 鎖 避免多執行緒互搶
public:
  bool open(const std::filesystem::path& logPath);
  void close();
  void log(LogLevel level,const std::string& message);
  bool isOpen() const noexcept{
    return output_.is_open();
  }
  void log(const ErrorSystem::ErrorInfo& info);
private:
  LogLevel mapErrorLevel(ErrorSystem::ErrorLevel errorlevel) const;
};