// ======================================================
// Module  : rtfconvert
// author  : suku156
// Purpose : 程式主入口，負責啟動 CLI 與 ConversionEngine
// layer   : main
//
// Depend(依賴對象) :
//   CliParser
//   ConversionEngine
//   ConversionRequestResolver
// Notes :
//   wmain 只負責流程啟動,不包含邏輯
// ==============================
#pragma execution_character_set("utf-8")
#include <io.h>       
#include <fcntl.h>    
#include <cstdio> 
#include "LogSystem_Module/LogSystem.h"
#include "ErrorSystem_Module/ErrorHandle.h"
#include "Universal_Module/Console.h"
#include "Cli_Module/CliParser.h"           // Cli 命令列選項模組 (將命令列資訊拆解成可用的容器)
#include "Cli_Module/ConversionEngine.h"   // Cli 模組 根據輸入的 Cli 決定如何呼叫主流程
#include "Cli_Module/ConversionRequestResolver.h" // Cli模組 轉譯層

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
  if(parseresult.showhelp){
    Cli::printHelp();
    return 0;
  }
  if(parseresult.shoeversion){
    Cli::printVersion();
    return 0;
  }

  if(!parseresult.ok){
    Console::ensureWcout(parseresult.message);
    Cli::printHelp();
    return 2;
  }
  
  // 將命令列解析後的資訊轉譯成預定型態
  Conversion::ResolvedConfig RLconfig = Conversion::resolveConfig(parseresult);

  // 用命令列訊息決定如何呼叫
  App::ConversionEngine conversionengine;
  App::AppExitCode resultCode = conversionengine.run(RLconfig);
  int result = static_cast<int>(resultCode);
  
  return result;
}
