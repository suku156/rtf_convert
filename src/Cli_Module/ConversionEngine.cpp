// =====================================================
// Module : ConversionEngine (implementation)
// =====================================================
#include "Cli_Module/ConversionEngine.h"
#include "Cli_Module/ConversionRequestResolver.h"
#include "Universal_Module/OutputDirGuard.h"
#include "MainProcess_Module/RTFProcessor.h"
#include "thread_Moudle/MyThread.h"
#include "Universal_Module/Console.h"
#include "MainProcess_Module/FileProcessRequest.h"
#include <system_error>
#include <filesystem>
#include <iostream>

namespace App{
  AppExitCode ConversionEngine::run(const Conversion::ResolvedConfig& RlConfig){
    
    auto input  = RlConfig.inputPath;
    auto output = RlConfig.outputDir;
    
    // 確認是否真的是可以用的input
    {
     std::error_code ec;
     auto st = std::filesystem::status(input,ec);
     if (ec) {
      std::wcout << L"[Error] 無法讀取輸入路徑: "
                 << input.wstring()
                 << L"\n";

      return AppExitCode::CliError;
     }
     if (!std::filesystem::exists(st)) {
      std::wcout << L"[Error] 輸入目標不存在: "
                 << input.wstring()
                 << L"\n";
      return AppExitCode::CliError;
     }
    }
    
    // 確保 outputDir 存在 使用原有模組達成
    OutputDirGuard fileOut(output);
    auto st = fileOut.checkDirectory(output);

    if(st != DirCheckResult::Ok){ // 檢查給定的指定資料夾
      std::wcout << L"輸出資料夾不符合規定,程式未執行\n";
      return AppExitCode::RunTimeError;
    }

    FileProcessRequest FPrequest;
    FPrequest.filePath = input;
    FPrequest.outputRootDir = output;
    FPrequest.outputFormat = RlConfig.format;
    FPrequest.dirPolicy = RlConfig.dirPolicy;
    
    std::error_code ec;
    //目標如果是單獨檔案的話
    if (std::filesystem::is_regular_file(input, ec)) {
      // 單檔：直接呼叫 processor
      RTFProcessor rtfprocessor;
      bool flag = rtfprocessor.processFile(FPrequest);
      if(flag){
        return AppExitCode::Success;  
      }else{
        return AppExitCode::Fail;
      }
    }
    else if (std::filesystem::is_directory(input, ec)) {
      RTFDirectoryRunner Drunner;
      ProgressObserver ProOB;
      Drunner.run(ProOB,FPrequest);  
    }

    return AppExitCode::Fail;
  }
}

