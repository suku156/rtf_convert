// ======================================================
// 共用小工具
// Notes :
//   用來確保輸出資料夾真的存在
//   沒有時會建立一個新的輸出資料夾
//   使用所有功能前先確保有至少先用過一次 ensure() 
// ==============================
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