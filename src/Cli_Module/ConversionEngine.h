// =====================================================
// Module  : CliParser
// Author  : suku156
// Purpose : 依據終端機輸入之資料決定如何呼叫
// Layer   : Cli
//
// Depend  :
//   ConversionRequestResolver
//   RTFProcessor
//   MyThread  
// Used by :
//   rtfconvert   
//
// Notes :
//   依據終端機給定的資料進行判斷如何呼叫主流程
// =====================================================

#pragma once

// forward declaration
namespace Conversion{
  struct  ResolvedConfig; 
}

namespace App{
  enum class AppExitCode{
    Success,
    Fail,
    CliError,
    RunTimeError
  };

  struct FileProcessRequest{
    std::filesystem::path filePath;         // 目前要處理的單一檔案
    std::filesystem::path outputRootDir;    // 輸出根目錄
    Common::OutputFormat outputFormat;      // 輸出格式
    Common::ProcessMode mode;                       // 單檔 / 批次 / 其他模式
    std::optional<std::filesystem::path> taskRootDir; // 批次任務根目錄，單檔可為 nullopt
    Common::ExistingDirPolicy dirPolicy;    // 已存在輸出資料夾的策略
  }; 

  class ConversionEngine{
  public:
  AppExitCode run(const Conversion::ResolvedConfig& RlConfig);
  private:
  }; 
}
