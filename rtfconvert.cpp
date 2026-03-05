#pragma execution_character_set("utf-8")
#include<iostream>
#include<fstream>
#include<string>
#include<sstream>
#include<vector>
#include<filesystem>
#include<fcntl.h>
#include<iconv.h>      
#include<algorithm>
#include<cstdlib>
#include<cctype>
#include<cstdint>
#include<optional>
#include<iomanip>
#include<thread>
#include<atomic>
#include<mutex>
#include<cstring>
#include "LogSystem_Module/LogSystem.h"
#include "ErrorSystem_Module/ErrorHandle.h"
#include "Universal_Module/Console.h"
#include "Cli_Module\CliParser.h"           // Cli 命令列選項模組 (將命令列資訊拆解成可用的容器)
#include "MainProcess_Module/RTFProcessor.h"// 主流程模組


#ifdef _WIN32
  #include <windows.h>
#endif

// 用來接收進度資訊並決定如何呈現的類別
class ProgressObserver{
  size_t total_{};
  std::atomic_size_t done_{0};
public:
  void Start(size_t num){
    total_ = num;
    done_.store(0,std::memory_order_relaxed);
    display();
  }

  void onUnitDone(){
    done_.fetch_add(1,std::memory_order_relaxed);
    display();
  }

  void Finish(){
    display();
    Console::ensureWcout( L" 已完成全部任務!\n");
  }

private:

  void display() const{
    Console::ensureWcout(L"已完成 " +
                std::to_wstring(done_.load(std::memory_order_relaxed)) +
                L" / 總數: " +
                std::to_wstring(total_));
  }
  
};

// 多執行緒任務的總管
class RTFDirectoryRunner{
public:
  void run(const std::filesystem::path& dirPath,ProgressObserver& ProOB){
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
          localprocessor.processFile(file,ProcessMode::BatchFile,dirPath);
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
  
private:
  std::vector<std::filesystem::path> collectRtfFiles(const std::filesystem::path& dirPath){
    std::vector<std::filesystem::path> files;
    for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
      if (entry.is_regular_file() && entry.path().extension() == L".rtf") {
        files.push_back(entry.path());
      }
    }
    return files;
  }

  size_t DecideThreadNum(size_t resultCount){
    if(resultCount == 0) return 0;

    size_t systemMax = std::thread::hardware_concurrency();
    if(systemMax == 0){ // 防呆
      systemMax = 4;
    }

    systemMax = std::min(systemMax,size_t(16)); // 最多就開十六條執行緒

    return std::min(systemMax,resultCount);
  }
};

int wmain(int argc,wchar_t* argv[]){
    // 強制讓 wcout 用 UTF-16 (Windows 本地寬字輸出)
    _setmode(_fileno(stdout), _O_U16TEXT);
    // 強制讓 wcerr 用 UTF-16 (Windows 本地寬字輸出)
    _setmode(_fileno(stderr), _O_U16TEXT);

    //主程序的日誌系統
    logSystem mainlogger;
    // 錯誤處理系統變數建立
    ErrorHandle mainerrorhandler(mainlogger);
    
    // 簡易防呆
    if(argc < 2){
      mainerrorhandler.handleFatalGlobal(L"需要正確輸入: "+std::wstring(argv[0])+ L" <檔案/資料夾路徑>\n");
    }

    // 命令列紀錄
    Cli::ParseResult parseresult = Cli::parse(argc,argv);
    if(!parseresult.ok){
      Console::ensureWcout(parseresult.message);
      Console::ensureWcout(Cli::usage());
      return 2;
    }
    

    //使用類別來執行任務
    RTFProcessor processor;
    std::filesystem::path filePath = parseresult.config.inputPath;
    

    // 分辨目標,依照目標屬性呼叫相對的函式
    
    /*
    if(std::filesystem::is_regular_file(filePath)){
      processor.processFile(filePath,ProcessMode::SingleFile);
    }else if(std::filesystem::is_directory(filePath)){
      RTFDirectoryRunner Drunner;
      ProgressObserver ProOB;
      Drunner.run(filePath,ProOB);
    }
    */  
    
    return EXIT_SUCCESS;
}
