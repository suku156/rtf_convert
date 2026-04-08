// =====================================================
// Module : MyThread (implementation)
// =====================================================
#include "MyThread.h"
#include "Universal_Module/Console.h"
#include "MainProcess_Module/RTFProcessor.h"
#include "MainProcess_Module/FileProcessRequest.h"
#include "Cli_Module/CliParser.h"
#include "I_O_Moudle/OutputPathResolver.h"
#include "Universal_Module/CommonEnum.h"
#include "Feedback_Module/IProgressObserver.h"
#include "Feedback_Module/ProgressEvent.h"
#include <thread>
#include <cstddef>
#include <system_error>
#include <filesystem>
#include <optional>
#include <iostream>
#include <string>

// 用來接收進度資訊並決定如何呈現的類別 (函式定義)
void ProgressObserver::Start(size_t num){
    total_ = num;
    done_.store(0,std::memory_order_relaxed);
    notify(ProgressEvent{
      ProgressEventType::BatchStart,
      L"開始多執行緒任務共有: "  + std::to_wstring(total_) + L" 個目標"
    });
}
void ProgressObserver::onUnitDone(){
    size_t current =  done_.fetch_add(1,std::memory_order_relaxed) + 1;
    notify(ProgressEvent{
      ProgressEventType::UnitDone,
      L"執行狀況 : ",
      current,
      total_
    });
}
void ProgressObserver::Finish(){
    notify(ProgressEvent{
      ProgressEventType::BatchFinish,
      L"多執行緒任務全部結束"
    });
}
void ProgressObserver::notify(const ProgressEvent& event){
  if(observer_){
    observer_->onEvent(event);
  }
}


// 多執行緒任務的總管 (函式定義)
void RTFDirectoryRunner::run(const FileProcessRequest& req,bool recursive, 
                             const OPResolver::ResolverRequest& templateResolverreq,
                             size_t threadCount)
  {
    // 1.建立工作清單
    std::vector<std::filesystem::path> files;
    files = collectRtfFiles(req.filePath,recursive);
    if(files.empty()) return;
    
    // 設定進度條的總數
    ProgressObserver ProOB(observer_);
    ProOB.Start(files.size());

    // 2.執行緒數量比對預設數量與實際需求
    size_t threadNum = ResolveThreadNum(files.size() , threadCount);
    if(threadNum == 0) return;//防呆
    
    // 3. atomic index 任務分配器 確保各執行緒之間不會因隨機執行搶任務
    std::atomic_size_t index{0};

    // 4.清空計數器
    successCount_.store(0,std::memory_order_relaxed);
    failCount_.store(0,std::memory_order_relaxed);

    // 一些由多執行緒總管持有的資源
    FileProcessRequest baseReq = req;
    // 檢查同名資料夾用的類別
    OPResolver::OutputPathRegistry registry;
    // 4. 用 lambda 執行某個任務
    auto worker = [&index,&files,&baseReq,&ProOB,this,&templateResolverreq,&registry](){
      // 給予空指標代表使用安靜模式此主流程不會傳遞主流程內資訊
      RTFProcessor  localprocessor(nullptr);
      OPResolver::OutputPathResolver resolver(registry);
      
      while(true){
        size_t i = index.fetch_add(1);
        if(i >= files.size()) break;

        const auto& file = files[i];
        FileProcessRequest fileReq = baseReq;
        fileReq.filePath = file;
        // 複製並補上 目標檔案路徑
        OPResolver::ResolverRequest useResolverreq = templateResolverreq;
        useResolverreq.inputFile = file;
        
        //負責的類別驗證路徑
        OPResolver::ResolverResult test = resolver.resolve(useResolverreq);
        if(!test.pathReserved){ // 檢查是否有同名檔案衝突問題
          failCount_.fetch_add(1, std::memory_order_relaxed);
          Console::ensureWcerr(L"路徑有同名衝突問題: " + file.wstring() + L"\n");
          ProOB.onUnitDone();
          continue;
        }
        fileReq.finalOutputPath = test.finalPath;
        fileReq.finalOutputDir  = test.parentDir; 
        
        try{
          bool ok = localprocessor.processFile(fileReq);
          if(ok){
            successCount_.fetch_add(1,std::memory_order_relaxed);
          }else{
            failCount_.fetch_add(1,std::memory_order_relaxed);
          }
        }catch(const std::exception& e){
          failCount_.fetch_add(1,std::memory_order_relaxed);
          Console::ensureWcerr(L"[Exception] " + file.wstring() + L"\n ");
        }
        catch(...){
          failCount_.fetch_add(1,std::memory_order_relaxed);
          Console::ensureWcerr(L"[Unknown Exception] " + file.wstring() +  L"\n");
        }
        
        
        ProOB.onUnitDone();
      }
    };

    // 5.建立 thread
    std::vector<std::thread> threads;
    threads.reserve(threadNum);

    for(size_t t =0; t < threadNum;t++){
      threads.emplace_back(worker);
    }

    // 6.確保所有 thread 結束
    for(auto& th:threads){
      th.join();
    }

    ProOB.Finish();
}
std::vector<std::filesystem::path> 
RTFDirectoryRunner::collectRtfFiles(
  const std::filesystem::path& dirPath ,bool recursive)
  {
    std::vector<std::filesystem::path> files;
    
    std::error_code ec;

    if(recursive){
      for (const auto& entry : std::filesystem::recursive_directory_iterator(dirPath, ec)) {
        
        if (ec) break;

        if (entry.is_regular_file() && entry.path().extension() == L".rtf") {
          files.push_back(entry.path());
        }
      }
 
    }else{
      for (const auto& entry : std::filesystem::directory_iterator(dirPath,ec)) {
        
        if(ec) break;
        
        if (entry.is_regular_file() && entry.path().extension() == L".rtf") {
          files.push_back(entry.path());
        }
      }
    }
    return files;
  }

size_t RTFDirectoryRunner::ResolveThreadNum(size_t fileCount,size_t threadCount)
{
  if(fileCount == 0) return 0;

  if(threadCount == 0){
    return 0; 
  }

  size_t result = std::min(threadCount, fileCount);
  
  return result;
}

size_t RTFDirectoryRunner::getSuccessNum() const{
  return successCount_.load(std::memory_order_relaxed);
}
size_t RTFDirectoryRunner::getFailNum() const{
  return failCount_.load(std::memory_order_relaxed);
}
