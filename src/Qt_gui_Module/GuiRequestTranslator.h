// =====================================================
// Module  : GuiRequestTranslator
// Author  : suku156
// Purpose : 負責將 gui 介面得到的資訊轉譯
// Layer   : gui
//
// Depend  :
//  GuiFormData
//  NormalizedConversionRequest
//
// Used by :
//  mainwindow
//
// Notes :
//   gui 中主要負責將介面得到的資訊轉換成核心功能使用的結構
// =====================================================

#ifndef GUIREQUESTTRANSLATOR_H
#define GUIREQUESTTRANSLATOR_H

#include "Task_Module/NormalizedConversionRequest.h"
#include "GuiFormData.h"
#include "Universal_Module/CommonEnum.h"
#include <string>
#include <filesystem>
#include <optional>
#include <cstddef>

struct GuiRequestResult{
    bool ok = false;
    std::wstring  message;
    NormalizedConversionRequest Normalizedrequest;
};

class GuiRequestTranslator{
public:
    GuiRequestResult translate(const GuiFormData& data);
private:
    std::optional<std::filesystem::path> checkAndToPath(const QString& s);
    std::optional<Common::OutputFormat> parseFormat(const QString& s);
    std::optional<Common::ExistingDirPolicy> parseDirPolicy(const QString& s);
    std::optional<size_t> threadStrToSizet(const QString& s);
    std::optional<bool> setPreserveRelativeStructure(const QString& s);
};

#endif // GUIREQUESTTRANSLATOR_H


