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
#include "I_O_Moudle/OutputPathResolver.h"
#include <system_error>
#include <filesystem>
#include <iostream>
#include <string>
#include <optional>

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
    OutputDirGuard fileOut(output,RlConfig.dirPolicy);
    auto st = fileOut.checkDirectory(output);

    if(st != DirCheckResult::Ok){ // 檢查給定的指定資料夾
      std::wcout << L"輸出資料夾不符合規定,程式未執行\n";
      return AppExitCode::RunTimeError;
    }

    // 主流程需要的參數資料結構
    FileProcessRequest FPrequest;
    FPrequest.filePath = input;
    FPrequest.outputRootDir = output;
    FPrequest.outputFormat = RlConfig.format;
    FPrequest.dirPolicy = RlConfig.dirPolicy;

    // 確保輸出檔案的資料結構
    OPResolver::ResolverRequest resolverReq;
    resolverReq.baseOutputDir = RlConfig.outputDir;
    switch(RlConfig.format){
      case Common::OutputFormat::Txt :
      resolverReq.format = OPResolver::OutputFormat::txt;
      break;
      case Common::OutputFormat::Md :
      resolverReq.format = OPResolver::OutputFormat::md;
      break;
      case Common::OutputFormat::Html :
      resolverReq.format = OPResolver::OutputFormat::html;
      break;
      default: // 預設使用 txt
      resolverReq.format = OPResolver::OutputFormat::txt;
      break;
    }
    switch(RlConfig.dirPolicy){
      case Common::ExistingDirPolicy::Reject :
      resolverReq.collisionPolicy = OPResolver::CollisionPolicy::ErrorIfExists;
      break;
      case Common::ExistingDirPolicy::Overwrite :
      resolverReq.collisionPolicy = OPResolver::CollisionPolicy::Overwrite;
      break;
      case Common::ExistingDirPolicy::RenameWithSuffix :
      resolverReq.collisionPolicy = OPResolver::CollisionPolicy::RenameWithSuffix;
      break;
      default: // 預設為安全模式
      resolverReq.collisionPolicy = OPResolver::CollisionPolicy::ErrorIfExists;
      break;
    }
    
    std::error_code ec;
    //目標如果是單獨檔案的話
    if (std::filesystem::is_regular_file(input, ec)) {
      // 依據判斷出來的模式再次調整 resolverReq
      resolverReq.inputFile = RlConfig.inputPath;
      resolverReq.taskRootDir = std::nullopt;
      resolverReq.mode = OPResolver::PathResolveMode::SingleFile;
      resolverReq.preserveRelativeStructure = false;

      //驗證得到的路徑並且將起寫入主流程的參數結構
      OPResolver::OutputPathResolver resolver;
      OPResolver::ResolverResult test = resolver.resolve(resolverReq);
      FPrequest.finalOutputPath = test.finalPath;
      FPrequest.finalOutputDir  = test.parentDir;

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
      // 依據判斷出來的模式再次調整 resolverReq
      // resolverReq.inputFile 先不設定資料夾模式要等到進函式內才會拿到
      resolverReq.taskRootDir = RlConfig.inputPath;// 給定輸入資料夾位置
      if(!RlConfig.recursive){
        resolverReq.mode = OPResolver::PathResolveMode::DirectoryFlat;
        resolverReq.preserveRelativeStructure = false;
      }else{
        resolverReq.mode = OPResolver::PathResolveMode::DirectoryRecursive;
        resolverReq.preserveRelativeStructure = true;
      }
      
      RTFDirectoryRunner Drunner;
      ProgressObserver ProOB;
      // 須注意 resolverReq.inputFile 在資料夾模式中需要進入多執行緒類別中設定,不然為空
      Drunner.run(ProOB,FPrequest,RlConfig.recursive,resolverReq);
      Console::ensureWcout(std::wstring(L"多執行緒成功數量: ") + 
                           std::to_wstring(Drunner.getSuccessNum()) + 
                           L" 多執行緒失敗數量: " + 
                           std::to_wstring(Drunner.getFailNum())+
                           L"\n");
      if(Drunner.getFailNum() == 0){
        return AppExitCode::Success;
      }
      else if(Drunner.getSuccessNum() == 0){
        return AppExitCode::RunTimeError;
      }
      return AppExitCode::PartialSuccess;                       
    }

    return AppExitCode::Fail;
  }
}

