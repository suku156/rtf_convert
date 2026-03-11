// ======================================================
// 給主流程用的參數
// 會將所有會用的參數全部打包
// ==============================

#pragma once
#include <filesystem>
#include <optional>
#include "Universal_Module/CommonEnum.h"

struct FileProcessRequest{
    std::filesystem::path filePath;         // 目前要處理的單一檔案
    std::filesystem::path outputRootDir;    // 輸出根目錄
    Common::OutputFormat outputFormat;      // 輸出格式
    Common::ProcessMode mode;                       // 單檔 / 批次 / 其他模式
    std::optional<std::filesystem::path> taskRootDir; // 批次任務根目錄，單檔可為 nullopt
    Common::ExistingDirPolicy dirPolicy;    // 已存在輸出資料夾的策略
};