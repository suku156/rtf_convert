// =====================================================
// Module : ConversionRequestResolver (implementation)
// =====================================================
#include "ConversionRequestResolver.h"
#include "Universal_Module/CommonEnum.h"
#include "CliParser.h"
#include <filesystem>
#include <system_error>
#include <string>
#include <cwctype>

Conversion::ResolvedConfig Conversion::resolveConfig(const Cli::ParseResult& parConfig){
  Conversion::ResolvedConfig resultconfig;

  // 先把相對路徑轉成絕對路徑
  std::filesystem::path input  = std::filesystem::absolute(parConfig.config.inputPath);
  std::filesystem::path output;
  std::error_code ec;
  if(parConfig.config.outputDir.has_value()){
    
    output = std::filesystem::absolute(
             parConfig.config.outputDir.value());

  }
  else{
    
    if (std::filesystem::is_regular_file(input, ec)){
      output = input.parent_path(); 
    }
    else if(std::filesystem::is_directory(input, ec)){
      output = input;  
    }
    else{
      output = input.parent_path();  
    }
  }

  resultconfig.inputPath = input;
  resultconfig.outputDir = output;
  // 處理 format
  if(parConfig.config.format.has_value()){
    resultconfig.format = parConfig.config.format.value();
  }else{
    resultconfig.format = Common::OutputFormat::Txt;
  }

  // 依據輸入決定資料夾處裡模式
  resultconfig.dirPolicy = parConfig.config.dirPolicy;

  // 依據指令決定是否要開啟遞迴輸入
  resultconfig.recursive = parConfig.recursive;

  return resultconfig;
}