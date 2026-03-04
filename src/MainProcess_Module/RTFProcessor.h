#pragma once
#include<filesystem>
#include<optional>

// 用來區分處理的是 單獨檔案 還是 資料夾
enum class ProcessMode{SingleFile,BatchFile};

// 主流程
class RTFProcessor{
public:
  void processFile(const std::filesystem::path& filePath,
                    ProcessMode mode,
                    std::optional<std::filesystem::path> taskRootDir = std::nullopt);
};


