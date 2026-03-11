// =====================================================
// Module : MyThread (implementation)
// =====================================================
#include "MyThread.h"
#include "Universal_Module/Console.h"
#include "MainProcess_Module/RTFProcessor.h"
#include "Cli_Module/CliParser.h"
#include <thread>


// 用來接收進度資訊並決定如何呈現的類別 (函式定義)
void ProgressObserver::Start(size_t num){
    total_ = num;
    done_.store(0,std::memory_order_relaxed);
    display();
}
void ProgressObserver::onUnitDone(){
    done_.fetch_add(1,std::memory_order_relaxed);
    display();
}
void ProgressObserver::Finish(){
    display();
    Console::ensureWcout( L" 已完成全部任務!\n");
}
void ProgressObserver::display() const{
    Console::ensureWcout(L"已完成 " +
                         std::to_wstring(done_.load(std::memory_order_relaxed)) +
                         L" / 總數: " +
                         std::to_wstring(total_));
}

// 多執行緒任務的總管 (函式定義)
void RTFDirectoryRunner::run(const std::filesystem::path& dirPath,
                             ProgressObserver& ProOB,
                             const std::filesystem::path& output,
                             Common::OutputFormat format){
    // 1.建立工作清單
    std::vector<std::filesystem::path> files;
    files = collectRtfFiles(dirPath);
    if(files.empty()) return;
    
    // 設定進度條的總數
    ProOB.Start(files.size());

    // 2.決定執行緒數量
    size_t threadNum = DecideThreadNum(files.size());
    if(threadNum == 0) return;//防呆

    // 3. atomic index 任務分配器 確保各執行緒之間不會因隨機執行搶任務
    std::atomic_size_t index{0};

    // 4. 用 lambda 執行某個任務
    auto worker = [&](){
      RTFProcessor  localprocessor;
      
      while(true){
        size_t i = index.fetch_add(1);
        if(i >= files.size()) break;

        const auto& file = files[i];

        try{
          localprocessor.processFile(file,output,format,ProcessMode::BatchFile,dirPath);
        }catch(const std::exception& e){
          Console::ensureWcerr(L"[Exception] " + file.wstring() + L"\n ");
        }
        catch(...){
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
std::vector<std::filesystem::path> RTFDirectoryRunner::collectRtfFiles(const std::filesystem::path& dirPath){
    std::vector<std::filesystem::path> files;
    for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
      if (entry.is_regular_file() && entry.path().extension() == L".rtf") {
        files.push_back(entry.path());
      }
    }
    return files;
}
size_t RTFDirectoryRunner::DecideThreadNum(size_t resultCount){
    if(resultCount == 0) return 0;

    size_t systemMax = std::thread::hardware_concurrency();
    if(systemMax == 0){ // 防呆
      systemMax = 4;
    }

    systemMax = std::min(systemMax,size_t(16)); // 最多就開十六條執行緒

    return std::min(systemMax,resultCount);
}