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
#include<optional>
#include<mutex>
#include "Universal_Module/CommonEnum.h"

// forward delecration
namespace OPResolver{
  struct ResolverRequest;
}
namespace Cli{
  struct ParseResult;
}
struct FileProcessRequest;
struct ProgressEvent;
class  IProgressObserver;

// 用來接收進度資訊並決定如何呈現的類別
class ProgressObserver{
  size_t total_{};
  std::atomic_size_t done_{0};
  IProgressObserver* observer_ = nullptr; 
public:
  explicit ProgressObserver(IProgressObserver* observer = nullptr) : observer_(observer) {}
  void Start(size_t num);
  void onUnitDone();
  void Finish();
private:
  void notify(const ProgressEvent& event);
  
};

// 多執行緒任務的總管
class RTFDirectoryRunner{
std::atomic<size_t> successCount_{0};
std::atomic<size_t> failCount_{0};
IProgressObserver* observer_ = nullptr;
std::mutex failedFilesMutex_;
std::vector<std::filesystem::path> failedFiles_;
public:
  explicit RTFDirectoryRunner(IProgressObserver* observer = nullptr) : observer_(observer) {}
  void run(const FileProcessRequest& req,bool recursive , 
           const OPResolver::ResolverRequest& templateResolverreq,
           size_t threadCount);
  size_t getSuccessNum() const;
  size_t getFailNum() const;
  bool hasFailedFiles() const;
  std::vector<std::filesystem::path> getFailedFiles() const;  
private:
  std::vector<std::filesystem::path> collectRtfFiles(const std::filesystem::path& dirPath,bool recursive);
  size_t ResolveThreadNum(size_t fileCount,size_t threadCount);
  void notify(const ProgressEvent& event);
  
};

