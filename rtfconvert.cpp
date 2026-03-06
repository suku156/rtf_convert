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
#include "Cli_Module/CliParser.h"           // Cli 命令列選項模組 (將命令列資訊拆解成可用的容器)
#include "Cli_Module/ConversionEngine.h"   // Cli 模組 根據輸入的 Cli 決定如何呼叫主流程


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

  // 用命令列訊息決定如何呼叫
  App::ConversionEngine conversionengine;
  App::AppExitCode resultCode = conversionengine.run(parseresult);
  int result = static_cast<int>(resultCode);
  
  return 0;
}
