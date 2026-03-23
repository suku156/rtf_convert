// =====================================================
// Module  : CliRequestResolver
// Author  : suku156
// Purpose : 負責轉換固定資料給執行層
// Layer   : Cli
//
// Depend  :
//   CliParser
//
// Used by :
//   ConversionEngine(要改)   
//
// Notes :
//   將 CliParser 解析得到的資料,清理成沒有 UI 痕跡的共用資料
// =====================================================
#pragma once
#include <filesystem>
#include <optional>
#include "Universal_Module/CommonEnum.h"

namespace Cli{
   struct ParseResult;
}

namespace Conversion {
  struct ResolvedConfig{
    std::filesystem::path inputPath;
    std::filesystem::path outputDir;
    Common::OutputFormat format;
    Common::ExistingDirPolicy dirPolicy = Common::ExistingDirPolicy::Reject;
    bool recursive = false;
    std::optional<bool> preserveRelativeStructure;
    std::optional<size_t> threadCount;
  };

  ResolvedConfig resolveConfig(const Cli::ParseResult& parConfig);
};

