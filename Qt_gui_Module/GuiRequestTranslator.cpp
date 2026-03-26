#include "GuiRequestTranslator.h"
#include "GuiFormData.h"
#include "Universal_Module/CommonEnum.h"
#include <filesystem>
#include <optional>


GuiRequestResult GuiRequestTranslator::translate(const GuiFormData& form){
    GuiRequestResult result;

    auto p = checkAndToPath(form.inputPath);
    if(!p){
        result.ok = false;
        result.message = L"缺少輸入路徑";
        return result;
    }
    result.Normalizedrequest.inputPath = *p;

    auto q = checkAndToPath(form.outputDir);
    if(!q){
        result.ok = false;
        result.message = L"缺少輸出資料夾路徑";
        return result;
    }
    result.Normalizedrequest.outputDir = *q;

    std::optional<Common::OutputFormat>fmt = parseFormat(form.formatText);
    if(!fmt){
        result.ok = false;
        result.message = L"不支援的輸出格式";
        return result;
    }
    result.Normalizedrequest.format = *fmt;

    result.ok = true;
    return result;
}

std::optional<std::filesystem::path> GuiRequestTranslator::checkAndToPath(const QString& s){
    if(s.trimmed().isEmpty()){
        return std::nullopt;
    }

    return std::filesystem::path(s.toStdWString());
}

std::optional<Common::OutputFormat> GuiRequestTranslator::parseFormat(const QString& s){
    QString t = s.trimmed().toLower();

    if(t == "txt") return Common::OutputFormat::Txt ;
    if(t == "md")  return Common::OutputFormat::Md  ;
    if(t == "html")return Common::OutputFormat::Html;

    return std::nullopt;
}