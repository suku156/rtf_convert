// =====================================================
// 用來對接 UI 資料 與 任務建構層 的結構
// =====================================================
#pragma once
#include <filesystem>
#include <optional>
#include "Universal_Module/CommonEnum.h"

struct NormalizedConversionRequest{
    std::filesystem::path inputPath;
    std::optional<std::filesystem::path> outputDir;
    std::optional<Common::OutputFormat> format;
    Common::ExistingDirPolicy dirPolicy = Common::ExistingDirPolicy::Reject;
    bool recursive = false;
    std::optional<bool> preserveRelativeStructure;
    // nullopt = 未指定，交由後續預設策略決定
    // true    = 保留相對中間路徑
    // false   = 不保留相對中間路徑
    std::optional<size_t> threadCount;
}; 