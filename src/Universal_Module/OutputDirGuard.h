#pragma once
#include<filesystem>

// 確保輸出的資料夾建立的工具
class OutputDirGuard{
  std::filesystem::path outputDir_;
  bool hasDir_ = false;
public:
  OutputDirGuard(const std::filesystem::path& outputDir) : outputDir_(outputDir) {}
  bool isReady() const;
  // 安全使用方法 : 在使用 ensure() 函式並回傳 true 後 可確保存在輸出資料夾
  const std::filesystem::path& path() const;
  bool ensure();
    
};