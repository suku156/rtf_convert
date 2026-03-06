#include "Cli_Module/ConversionEngine.h"
#include "Cli_Module/CliParser.h"
#include "Universal_Module/OutputDirGuard.h"
#include "MainProcess_Module/RTFProcessor.h"
#include "thread_Moudle/MyThread.h"
#include <system_error>
#include <filesystem>

namespace App{
  AppExitCode ConversionEngine::run(const Cli::ParseResult& pr){
    // 確認是否真的是可以用的input
    {
     std::error_code ec;
     auto st = std::filesystem::status(pr.config.inputPath,ec);
     if (ec) {
       return AppExitCode::CliError;
     }
     if (!std::filesystem::exists(st)) {
       return AppExitCode::CliError;
     }
    }
    
    // 確保 outputDir 存在 使用原有模組達成
    OutputDirGuard fileOut(pr.config.outputDir);
    if(!fileOut.ensure()){ // ensure 後沒有回傳 false 就可以確保有輸出資料夾
      return AppExitCode::RunTimeError;
    }

    //下一階段 不指定 output 版本可以用
    //std::filesystem::path outputDir = filePath.parent_path() / baseName;

    std::error_code ec;
    //目標如果是單獨檔案的話
    if (std::filesystem::is_regular_file(pr.config.inputPath, ec)) {
      // 單檔：呼叫 processor（可能要多傳 out/fmt）
      RTFProcessor rtfprocessor;
      rtfprocessor.processFile(pr.config.inputPath,
                               pr.config.outputDir,
                               pr.config.format,
                               ProcessMode::SingleFile);
      return AppExitCode::Success;  
    }
    else if (std::filesystem::is_directory(pr.config.inputPath, ec)) {
        
    }

    return AppExitCode::GeneralError;
  }
}

