// =====================================================
// Module : CliRequestResolver (implementation)
// =====================================================
#include "CliRequestResolver.h"
#include "Universal_Module/CommonEnum.h"
#include "Task_Module/NormalizedConversionRequest.h"
#include "CliParser.h"
#include <filesystem>
#include <system_error>
#include <string>
#include <cwctype>

NormalizedConversionRequest Conversion::resolveConfig(const Cli::ParseResult& parConfig){
  Conversion::ResolvedConfig resultconfig;

  // 先把相對路徑轉成絕對路徑
  std::filesystem::path input  = std::filesystem::absolute(parConfig.config.inputPath);
  
  NormalizedConversionRequest request;
  request.inputPath = input;
  request.outputDir = parConfig.config.outputDir;
  request.format    = parConfig.config.format;
  request.dirPolicy = parConfig.config.dirPolicy;
  request.recursive = parConfig.recursive;
  request.preserveRelativeStructure = parConfig.config.preserveRelativeStructure;
  request.threadCount = parConfig.config.threadCount;

  return request;
}