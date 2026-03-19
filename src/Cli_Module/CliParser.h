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
//   ConversionRequestResolver   
//
// Notes :
//   只負責解析輸入資料,不做轉換.判斷或執行
// =====================================================
#pragma once
#include "Universal_Module/CommonEnum.h"
#include <filesystem>
#include <string>
#include <optional>
#include <cstddef>

namespace Cli{
  
  // 核心資訊
  struct ParsedConfig{
    std::filesystem::path inputPath;
    std::optional<std::filesystem::path> outputDir;
    std::optional<Common::OutputFormat> format;
    Common::ExistingDirPolicy dirPolicy = Common::ExistingDirPolicy::Reject;
    std::optional<bool> preserveRelativeStructure;
    std::optional<size_t> threadCount;
  };

  //真正使用的資訊
  struct ParseResult{
    bool showhelp = false;
    bool shoeversion = false;
    bool recursive = false;
    bool ok = false;
    ParsedConfig config;
    std::wstring message;
  };

  // 將得到的訊息拆解成 ParseResult 回傳給主流程去做使用
  ParseResult parse(int argc,wchar_t* argv[]);
  // 用來顯示 --help 功能介紹
  void printHelp();
  // 用來顯示 --version 版本介紹
  void printVersion();
}