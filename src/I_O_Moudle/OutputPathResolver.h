// =====================================================
// Module  : OutputPathResolver
// Author  : suku156
// Purpose : 確定輸出檔案名稱
// Layer   : I/O
//
// Depend  :
//   Console  
//
// Used by :
//   ConversionEngine  
//   Mythread
//
// Notes :
//   用於確保輸出檔案
//   如果是資料夾遞迴模式可以放在不同資料夾時確保其會在各自的資料夾
//   如果要放在同一個資料夾要確保會依照資料夾同名策略處理的情況
// =====================================================
#pragma once
#include <filesystem>
#include <optional>
#include <mutex>
#include <unordered_set>

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

  // 類別用來回傳的結果
  struct ResolverResult{
    std::filesystem::path finalPath;
    std::filesystem::path parentDir;
    std::filesystem::path relativeSubDir;
    bool pathReserved = false;
  };

  // 共用的輸出資料夾名稱分配器
  class OutputPathRegistry{
    std::mutex mutex_;
    std::unordered_set<std::wstring> reserved_;
  public:
    std::optional<std::filesystem::path> reserveUniqueDir(const std::filesystem::path& parent,
                                                          const std::wstring& baseName,
                                                          CollisionPolicy policy);
  private:
    // 使用前提:呼叫者必須已經持有 : mutex_;
    // 此函式會存取 reserved_，因此不可在未上鎖情況下呼叫。
    std::optional<std::filesystem::path> pathToSuffixLoopLocked(const std::filesystem::path& parent,
                                                          const std::wstring& baseName);                                                         
  };
  
  // 負責處裡的類別
  class OutputPathResolver{
    OutputPathRegistry& registry_;
  public:
    OutputPathResolver(OutputPathRegistry& registry) : registry_(registry) {}
    ResolverResult resolve(const ResolverRequest& request) const;  
  
  private:
    // 再資料夾遞迴模式下用來算出子資料夾與目標資料夾中間的路徑
    std::filesystem::path buildRelativeSubDir(const ResolverRequest& request) const;
  };

  
  
}
