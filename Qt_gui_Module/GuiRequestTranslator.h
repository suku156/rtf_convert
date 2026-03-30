#ifndef GUIREQUESTTRANSLATOR_H
#define GUIREQUESTTRANSLATOR_H

#endif // GUIREQUESTTRANSLATOR_H
#pragma once
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
