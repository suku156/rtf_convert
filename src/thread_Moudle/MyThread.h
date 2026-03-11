// =====================================================
// Module  : MyThread
// Author  : suku156
// Purpose : 方案的多執行緒功能
// Layer   : thread
//
// Depend  :
//   RTFProcessor
//   Console
//   CliParser
//
// Used by :
//   ConversionEngine   
//   
// Notes :
//   目標是資料夾時啟用多執行緒
//   每個執行緒負責一個檔案,各自呼叫主流程
// =====================================================
#pragma once
#include<cstddef>
#include<atomic>
#include<filesystem>
#include<vector>
#include "Universal_Module/CommonEnum.h"

// forward delecration
namespace Cli{
  struct ParseResult;
}

// 用來接收進度資訊並決定如何呈現的類別
class ProgressObserver{
  size_t total_{};
  std::atomic_size_t done_{0}; 
public:
   void Start(size_t num);
   void onUnitDone();
   void Finish();
private:
   void display() const;
};

// 多執行緒任務的總管
class RTFDirectoryRunner{
public:
  void run(const std::filesystem::path& dirPath,ProgressObserver& ProOB,
           const std::filesystem::path& output,Common::OutputFormat format);
private:
  std::vector<std::filesystem::path> collectRtfFiles(const std::filesystem::path& dirPath);
  size_t DecideThreadNum(size_t resultCount);
};

