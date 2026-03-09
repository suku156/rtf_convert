// =====================================================
// Module  : CliParser
// Author  : suku156
// Purpose : 負責解析終端機輸入之資料
// Layer   : Cli
//
// Depend  :
//   CommonEnum
//
// Used by :
//   rtfconvert   
//
// Notes :
//   只負責解析輸入資料,不做轉換.判斷或執行
// =====================================================
#pragma once
#include "Universal_Module/CommonEnum.h"
#include <filesystem>
#include <string>

namespace Cli{
  
  // 核心資訊
  struct Config{
    std::filesystem::path inputPath;
    std::filesystem::path outputDir;
    OutputFormat format = OutputFormat::Txt;
  };

  //真正使用的資訊
  struct ParseResult{
    bool ok = false;
    Config config;
    std::wstring message;
  };

  // 將得到的訊息拆解成 ParseResult 回傳給主流程去做使用
  ParseResult parse(int argc,wchar_t* argv[]);
  // 正確範例.在打錯時告知使用者正確的版本
  std::wstring usage();
}