#include "GuiRequestTranslator.h"
#include "GuiFormData.h"
#include "Universal_Module/CommonEnum.h"
#include <filesystem>
#include <optional>


GuiRequestResult GuiRequestTranslator::translate(const GuiFormData& form){
    GuiRequestResult result;

    // 輸入目標檔案一定要給
    auto p = checkAndToPath(form.inputPath);
    if(!p){
        result.ok = false;
        result.message = L"缺少輸入路徑";
        return result;
    }
    result.Normalizedrequest.inputPath = *p;

    // 輸出資料夾允許不指定
    result.Normalizedrequest.outputDir = checkAndToPath(form.outputDir);

    // 允許不指定型態
    result.Normalizedrequest.format = parseFormat(form.formatText);

    result.ok = true;
    return result;
}

std::optional<std::filesystem::path> GuiRequestTranslator::checkAndToPath(const QString& s){

    QString trimmed = s.trimmed();
    if(trimmed.isEmpty()){
        return std::nullopt;
    }

    return std::filesystem::absolute(std::filesystem::path(trimmed.toStdWString()));
}

std::optional<Common::OutputFormat> GuiRequestTranslator::parseFormat(const QString& s){
    QString t = s.trimmed().toLower();

    if(t == "txt") return Common::OutputFormat::Txt ;
    if(t == "md")  return Common::OutputFormat::Md  ;
    if(t == "html")return Common::OutputFormat::Html;
    if(t == "default") return std::nullopt;

    return std::nullopt;
}