// ======================================================
// 共用小工具
// Notes :
//   用來確保建立輸出資料夾真的存在
//   
//   ensure() 一次後就可以使用 path 功能
// ==============================
#pragma once
#include<filesystem>

enum class EnsureDirResult{
  Success,
  AlreadyExists,
  NotDirectory,
  CreateFailed,
  VerifyFailed
};

enum class DirCheckResult {
  Ok,
  NotExist,
  NotDirectory,
  StatusFailed
};

// 確保輸出的資料夾建立的工具
class OutputDirGuard{
  std::filesystem::path outputDir_;
  bool hasDir_ = false;
public:
  OutputDirGuard(const std::filesystem::path& outputDir) : outputDir_(outputDir) {}
  bool isReady() const;
  // 安全使用方法 : 在使用 ensure() 函式並回傳 Success 後 可確保存在輸出資料夾
  const std::filesystem::path& path() const;
  EnsureDirResult ensure();
  DirCheckResult checkDirectory(const std::filesystem::path& p);  
};