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