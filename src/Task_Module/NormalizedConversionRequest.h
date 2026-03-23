// =====================================================
// 用來對接 UI 資料 與 任務建構層 的結構
// =====================================================
#pragma once
#include <filesystem>
#include <optional>

struct NormalizedConversionRequest{
    std::filesystem::path inputPath;
    std::filesystem::path outputDir;
    Common::OutputFormat format;
    Common::ExistingDirPolicy dirPolicy = Common::ExistingDirPolicy::Reject;
    bool recursive = false;
    std::optional<bool> preserveRelativeStructure;
    std::optional<size_t> threadCount;
}; 