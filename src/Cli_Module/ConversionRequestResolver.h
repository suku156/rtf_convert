// =====================================================
// Module  : ConversionRequestResolver
// Author  : suku156
// Purpose : 負責轉換固定資料給執行層
// Layer   : Cli
//
// Depend  :
//   CliParser
//
// Used by :
//   ConversionEngine   
//
// Notes :
//   將 CliParser 解析得到的資料,確定成固定形式給 ConversionEngine 使用
// =====================================================
#pragma once
#include <filesystem>
#include "Universal_Module/CommonEnum.h"

namespace Cli{
   struct ParseResult;
   enum class OutputFormat;
};

namespace Conversion {
  struct ResolvedConfig{
    std::filesystem::path inputPath;
    std::filesystem::path outputDir;
    Common::OutputFormat format;
  };

  ResolvedConfig resolveConfig(const Cli::ParseResult& parConfig);
};

