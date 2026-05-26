// =====================================================
// Module : ConversionExecutor (implementation)
// =====================================================
#include "ConversionExecutor.h"
#include "Cli_Module/CliRequestResolver.h"
#include "Task_Module/ConversionTask.h"
#include "Universal_Module/OutputDirGuard.h"
#include "MainProcess_Module/RTFProcessor.h"
#include "thread_Moudle/MyThread.h"
#include "Universal_Module/Console.h"
#include "MainProcess_Module/FileProcessRequest.h"
#include "I_O_Moudle/OutputPathResolver.h"
#include "Feedback_Module/ProgressEvent.h"
#include <system_error>
#include <filesystem>
#include <iostream>
#include <string>
#include <optional>



namespace App{
  AppExitCode ConversionEngine::run(const BuildResult& result){
    
    if(!result.ok){
      std::wcout << L"[Error]" << result.message << L"\n";
      return AppExitCode::Fail;
    }
    notify(ProgressEvent{
      ProgressEventType::Start,
      L"啟動轉換流程"
    });
    
    auto task = result.task;

    auto input  = task.inputPath;
    auto output = task.outputDir;
    
    // 確認是否真的是可以用的input
    {
     std::error_code ec;
     auto st = std::filesystem::status(input,ec);
     if (ec) {
      std::wcout << L"[Error] 無法讀取輸入路徑: "
                 << input.wstring()
                 << L"\n";

      return AppExitCode::Fail;
     }
     if (!std::filesystem::exists(st)) {
      std::wcout << L"[Error] 輸入目標不存在: "
                 << input.wstring()
                 << L"\n";
      return AppExitCode::Fail;
     }
    }
    
    // 確保 outputDir 存在 使用原有模組達成
    OutputDirGuard fileOut(output,task.dirPolicy);
    auto st = fileOut.checkDirectory(output);

    if(st != DirCheckResult::Ok){ // 檢查給定的指定資料夾
      std::wcout << L"輸出資料夾不符合規定,程式未執行\n";
      return AppExitCode::RunTimeError;
    }

    // 主流程需要的參數資料結構
    FileProcessRequest FPrequest;
    FPrequest.filePath = input;
    FPrequest.outputRootDir = output;
    FPrequest.outputFormat = task.format;
    FPrequest.dirPolicy = task.dirPolicy;

    // 確保輸出檔案的資料結構
    OPResolver::ResolverRequest resolverReq;
    resolverReq.baseOutputDir = task.outputDir;
    switch(task.format){
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
    switch(task.dirPolicy){
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
      resolverReq.inputFile = task.inputPath;
      resolverReq.taskRootDir = std::nullopt;
      resolverReq.mode = OPResolver::PathResolveMode::SingleFile;
      resolverReq.preserveRelativeStructure = false;

      //驗證得到的路徑並且將起寫入主流程的參數結構
      // 檢查同名資料夾用的類別
      OPResolver::OutputPathRegistry registry;
      OPResolver::OutputPathResolver resolver(registry);
      OPResolver::ResolverResult test = resolver.resolve(resolverReq);
      if(!test.pathReserved){
        Console::ensureWcout(L"因為資料夾同名碰裝問題程式中止!!");
        return AppExitCode::Fail;
      }
      FPrequest.finalOutputPath = test.finalPath;
      FPrequest.finalOutputDir  = test.parentDir;

      // 單檔：直接呼叫 processor
      RTFProcessor rtfprocessor(observer_);
      ProcessResult flag = rtfprocessor.processFile(FPrequest);
      switch(flag){
        case ProcessResult::SuccessClean : 
          notify(ProgressEvent{
            ProgressEventType::Finish,
            L"轉換成功"
          });
          return AppExitCode::Success; 
        case ProcessResult::Failed :
          notify(ProgressEvent{
            ProgressEventType::Finish,
            L"轉換失敗"
          });
          return AppExitCode::Fail;
        case ProcessResult::SuccessWithWarning:
          notify(ProgressEvent{
            ProgressEventType::Finish,
            L"轉換完成,但包含警告,請查看 log"
          });
          return AppExitCode::PartialSuccess;
      }

      notify(ProgressEvent{
        ProgressEventType::Error,
        L"轉換結果狀態未知"
      });
      return AppExitCode::RunTimeError;
    }
    else if (std::filesystem::is_directory(input, ec)) {
      // 依據判斷出來的模式再次調整 resolverReq
      // resolverReq.inputFile 先不設定資料夾模式要等到進函式內才會拿到
      resolverReq.taskRootDir = task.inputPath;// 給定輸入資料夾位置
      
      // 依據有無遞迴調整模式
      resolverReq.mode = task.recursive
      ? OPResolver::PathResolveMode::DirectoryRecursive
      : OPResolver::PathResolveMode::DirectoryFlat;

      // 決定子資料夾的輸出資料夾是否要保有中間路徑
      // 先依照使用者設定決定沒有就用預設行為
      if(!task.recursive){ // 非遞迴模式會無視保留中間值的指令
        resolverReq.preserveRelativeStructure = false;
      }else{
        resolverReq.preserveRelativeStructure = task.preserveRelativeStructure;
      }
      
      RTFDirectoryRunner Drunner(observer_);
      // 須注意 resolverReq.inputFile 在資料夾模式中需要進入多執行緒類別中設定,不然為空
      Drunner.run(FPrequest,task.recursive,resolverReq,task.threadCount);
      notify(ProgressEvent{
        ProgressEventType::Info,
        std::wstring(L"多執行緒成功數量: ") + std::to_wstring(Drunner.getSuccessNum()) + 
                     L" 警告數量: " + std::to_wstring(Drunner.getWarningNum())  +  
                     L" 失敗數量: " + std::to_wstring(Drunner.getFailNum())
      });
      if(Drunner.hasFailedFiles()){
        std::vector<std::filesystem::path> failedFiles = Drunner.getFailedFiles();
        notify(ProgressEvent{
          ProgressEventType::Error,
          L"以下檔案轉換失敗:"
        });
        for(const auto& path : failedFiles){
          notify(ProgressEvent{
            ProgressEventType::Detail,
            path.filename().wstring()
          });
        }
        notify(ProgressEvent{
          ProgressEventType::Info,
          L"詳細錯誤請查看輸出資料夾內的 log.txt"
        });
      }

      if(Drunner.hasWarningFiles()){
        notify(ProgressEvent{
          ProgressEventType::Warning,
          L"以下檔案轉換成功但包含警告，詳細資訊請查看 log"
        });

        for(const auto& p:Drunner.getWarningFiles()){
          notify(ProgressEvent{
            ProgressEventType::Detail,
            p.filename().wstring()
          });
        }
      }

      size_t failNum = Drunner.getFailNum();
      size_t successNum = Drunner.getSuccessNum();
      size_t warningNum = Drunner.getWarningNum();

      // 全部失敗
      if(successNum == 0){
        notify(ProgressEvent{
          ProgressEventType::Finish,
          L"多執行緒轉換失敗"
        });

        return AppExitCode::RunTimeError;
      }

      // 沒有失敗也沒有警告
      if(failNum == 0 && warningNum == 0){
        notify(ProgressEvent{
          ProgressEventType::Finish,
          L"多執行緒轉換成功"
        });

        return AppExitCode::Success;
      }

      // 有警告但沒有失敗
      if(failNum == 0 && warningNum > 0){
        notify(ProgressEvent{
          ProgressEventType::Finish,
          L"多執行緒轉換成功，但包含警告，請查看 log"
        });

        return AppExitCode::PartialSuccess;
      }

      // 剩下就是部份失敗
      notify(ProgressEvent{
        ProgressEventType::Finish,
        L"多執行緒轉換部份成功"
      });

      return AppExitCode::PartialSuccess;
    }

    return AppExitCode::Fail;
  }
}

void App::ConversionEngine::notify(const ProgressEvent& event){
  if(observer_){
    observer_->onEvent(event);
  }
}