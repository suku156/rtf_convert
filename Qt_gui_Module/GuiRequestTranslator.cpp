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

    // 傳遞是否遞迴
    result.Normalizedrequest.recursive = form.recursive;

    // 將輸出資料夾同名處裡模式轉換成enum
    std::optional<Common::ExistingDirPolicy> dirPolicyOpt = parseDirPolicy(form.dirPolicy);
    if(!dirPolicyOpt){
        result.ok = false;
        result.message = L"資料夾同名處裡策略無效,請重新選擇";
        return result;
    }
    result.Normalizedrequest.dirPolicy = *dirPolicyOpt;

    // 將執行緒數量由字串轉為預定型態
    result.Normalizedrequest.threadCount = threadStrToSizet(form.threadtext);

    // 設定遞迴模式下是否保留輸出資料夾中間路徑
    result.Normalizedrequest.preserveRelativeStructure = setPreserveRelativeStructure(form.preserveRelativeStructure);

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

std::optional<Common::ExistingDirPolicy> GuiRequestTranslator::parseDirPolicy(const QString& s){
   QString t = s.trimmed().toLower();

   if(t == "reject")           return Common::ExistingDirPolicy::Reject;
   if(t == "overwrite")        return Common::ExistingDirPolicy::Overwrite;
   if(t == "renamewithsuffix") return Common::ExistingDirPolicy::RenameWithSuffix;

   return std::nullopt;
}

std::optional<size_t> GuiRequestTranslator::threadStrToSizet(const QString& s){
    QString t = s.trimmed();

    if(t.isEmpty()){
        return std::nullopt;
    }

    bool ok = false;
    int v = t.toInt(&ok);

    // 轉換失敗或輸入值有錯誤就走預設路線
    if(!ok || v <= 0 || v > 16){
        return std::nullopt;
    }

    return static_cast<size_t>(v);
}

std::optional<bool> GuiRequestTranslator::setPreserveRelativeStructure(const QString& s){
    QString t = s.trimmed().toLower();

    if(t == "preserve-structure") return true;
    if(t == "flat-output")return false;
    if(t == "default") return std::nullopt;

    return std::nullopt;
}

