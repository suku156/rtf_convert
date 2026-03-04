#include "Cli_Module/ConversionEngine.h"
#include "Cli_Module/CliParser.h"
#include "Universal_Module/OutputDirGuard.h"
#include <system_error>

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

    

  }
}

