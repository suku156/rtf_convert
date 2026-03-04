#pragma once
#include <filesystem>
//#include <optional>
#include <string>

namespace Cli{
  // 表示輸出格式
  enum class OutputFormat{
    Txt,
    Md,
    Html
  };

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