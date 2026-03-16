// =====================================================
// Module  : OutputPathResolver
// Author  : suku156
// Purpose : 確定輸出檔案名稱
// Layer   : I/O
//
// Depend  :
//
// Used by :
//   ConversionEngine  
//
// Notes :
//   用於確保輸出檔案
//   如果是資料夾遞迴模式可以放在不同資料夾時確保其會在各自的資料夾
//   如果要放在同一個資料夾要確保不會有同名的情況
// =====================================================
#pragma once
#include <filesystem>
#include <optional>

namespace OPResolver{
  // 用來表示現在的處理模式
  enum class PathResolveMode {
    SingleFile,
    DirectoryFlat,
    DirectoryRecursive
  };

  // 用來表示輸出格式
  enum class OutputFormat{
    txt,
    md,
    html,
  };

  // 用來表示發現衝突(重名)後的處理策略
  enum class CollisionPolicy{
    Overwrite,
    RenameWithSuffix,
    ErrorIfExists
  };

  // 真正在使用的結構
  struct ResolverRequest{
    // inputFile 代表目前正在處理的單一輸入檔案的完整路徑
    std::filesystem::path inputFile;
    // baseOutputDir Cli 給定的輸出根目錄
    std::filesystem::path baseOutputDir;
    PathResolveMode mode = PathResolveMode::SingleFile;
    OutputFormat format = OutputFormat::txt;
    // 批次任務的輸入根目錄
    // 用來計算 relative path 
    std::optional<std::filesystem::path> taskRootDir;
    // 用來決定是否保留多資料夾的結構來放置輸出檔案
    // 遞迴模式有開才會用到
    bool preserveRelativeStructure = true;
    CollisionPolicy collisionPolicy = CollisionPolicy::RenameWithSuffix;
  };
  
}
