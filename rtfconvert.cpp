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
