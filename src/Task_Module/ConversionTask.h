// =====================================================
// 用來對接 任務建構層 與 執行層 的結構
// =====================================================
#pragma once
#include <filesystem>
#include <optional>
#include <stddef.h>
#include <string>
#include <Universal_Module/CommonEnum.h>

// 核心資訊
struct ConversionTask{
  std::filesystem::path inputPath;
  std::filesystem::path outputDir;
  // 輸出檔格式
  Common::OutputFormat format;
  // 遇到同名檔案的處理策略
  Common::ExistingDirPolicy dirPolicy = Common::ExistingDirPolicy::Reject;
  // 遞迴模式
  bool recursive = false;
  // 遞迴模式是否保留中間的資料夾路徑
  bool preserveRelativeStructure;
  // 用來表示使用者有無指定使用的執行緒數量
  size_t threadCount; 
};

// 實際傳遞之資訊
struct BuildResult{
  bool ok = true;
  std::wstring message;
  ConversionTask task;
};