// =====================================================
// Module : LogSystem (implementation)
// 設計時有加入鎖的概念進行確保
// =====================================================
#include "LogSystem.h"
#include "ErrorSystem_Module/ErrorSystem.h"
#include<system_error>
#include<fstream>
#include<mutex>
#include<filesystem>
#include<string>
#include<chrono>
#include<ctime>
#include<sstream>
#include<iomanip>


bool logSystem::open(const std::filesystem::path& logPath){
    std::lock_guard<std::mutex> lock(mutex_); // 開啟時先上鎖 使用lock_guard 會在週期結束自動解鎖
    
    if(logPath.empty()) return false; //防呆
    
    //檔案如果已經開啟了 先關閉
    if(output_.is_open()){
      output_.close();
    }

    // 確保目錄存在
    std::error_code ec;
    std::filesystem::create_directories(logPath, ec);
    if (ec) {
      return false;
    }
    
    std::filesystem::path logout = logPath / "log.txt";
    
    output_.open(logout, std::ios::out | std::ios::app);
    if(!output_.is_open()) return false;
    
    
    //先寫一小段文字做測試
    output_ << "[INFO] log system opened\n";
    output_.flush();

    return true;
}

void logSystem::close(){
    std::lock_guard<std::mutex> lock(mutex_);
    if(output_.is_open()) {
      output_ << "[INFO] log system closed\n";
      output_.flush();
      output_.close();
    }
}

void logSystem::log(LogLevel level,const std::string& message){
    std::lock_guard<std::mutex> lock(mutex_);

    if(!output_.is_open()) return;

    //取得現有時間
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);

    std::tm tm{};
  #ifdef _WIN32
    localtime_s(&tm,&t);
  #else 
    localtime_r(&t,&tm);
  #endif 

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");

    //將輸入的 LogLevel 轉化成字串
    const char* levelStr = nullptr;
    switch(level){
      case LogLevel::Info  : levelStr = "INFO" ;break;
      case LogLevel::Warn  : levelStr = "WARN" ;break;
      case LogLevel::Error : levelStr = "ERROR";break;
      case LogLevel::Debug : levelStr = "Debug";break; 
    }

    // 將所有訊息輸入目標檔案中
    output_ << "[" << oss.str() << "] "
            << "[" << levelStr  << "] "
            << message
            << "\n";
    
    output_.flush();
} 

void logSystem::log(const ErrorSystem::ErrorInfo& info){
    for(const auto& item:info.getItems()){
      
      LogLevel level = mapErrorLevel(item.level);

      std::string msg;
      msg.reserve(128);

      msg += "[";
      msg += ErrorSystem::getCategoryStr(item.category);
      msg += "] ";

      msg += "[";
      msg += ErrorSystem::getTypeStr(item.type);
      msg += "] ";

      msg += ErrorSystem::toUtf8(item.message);

      log(level,msg);  
    }
}

LogLevel logSystem::mapErrorLevel(ErrorSystem::ErrorLevel errorlevel) const{
    switch(errorlevel){
      case ErrorSystem::ErrorLevel::None:
      return LogLevel::Info;
      break;
      case ErrorSystem::ErrorLevel::Warning:
      return LogLevel::Warn;
      break;
      case ErrorSystem::ErrorLevel::Recoverable:
      case ErrorSystem::ErrorLevel::FatalLocal:
      case ErrorSystem::ErrorLevel::FatalGlobal:
      return LogLevel::Error;
      break;
    }

    return LogLevel::Error;
}