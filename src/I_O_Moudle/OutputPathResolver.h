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
    wmf,
    png
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
    bool preserveRelativeStructure = true;
    CollisionPolicy collisionPolicy = CollisionPolicy::RenameWithSuffix;
  };
  
}
