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
    Common::ExistingDirPolicy dirPolicy;    // 已存在輸出資料夾的策略
    std::filesystem::path finalOutputPath;  // resolver 模組計算出的最終檔案完整路徑
    std::filesystem::path finalOutputDir;   // resolver 模組計算出的最終輸出檔案所在的資料夾路徑 
};