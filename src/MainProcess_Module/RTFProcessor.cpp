// =====================================================
// Module : RTFProcessor (implementation)
// =====================================================
#include "RTFProcessor.h"
#include "Universal_Module/OutputDirGuard.h" // 用來確保輸出資料夾的模組
#include "Universal_Module/Console.h"        // 用來確保多執行緒的情況下 std::wcout 以及 std::wcerr 不會互相交叉
#include "LogSystem_Module/LogSystem.h"   // 日誌系統的模組
#include "ErrorSystem_Module/ErrorHandle.h"   // 決定如何處裡錯誤的模組
#include "ErrorSystem_Module/ErrorSystem.h" // 放置錯誤系統資訊
#include "FormatDetection_Module/RtfFormatDetection.h" // 偵測並判斷 rtf 格式與基礎編碼的模組
#include "PictProcessor_Module/pictProcessor.h"  // 處裡圖片的模組
#include "ErrorDetector_Module/ErrorDetector.h" // 錯誤偵測的模組
#include "EncodingProcess_Module/Decode.h"        // 解碼模組
#include "EncodingProcess_Module/GroupProcessor.h" // 處裡 utf 體系群組的模組
#include "TextProcessor_Module/textProcessor.h"  // 用來處理解碼完的文字
#include "SemanticStructure_Module/Document.h"       // 語意結構與轉化成語意結構用的模組
#include "SemanticStructure_Module/Renderer.h"       // 依據需求解讀語意結構用的模組
#include "Universal_Module/CommonEnum.h"  // 多個模組共用的 enum
#include<filesystem>
#include<optional>
#include<iostream>
#ifdef _WIN32
  #include <windows.h>
#endif

// 使用 namespace 的隱常功能
// 目前只要主流程會用到的類別與函式,先放在.cpp 中 有需求時在拆出去
namespace{
   // 用來檢查與替換檔名中有影響的空格
  std::wstring sanitizeFileName(const std::wstring& basename){
    std::wstring result;
    result.reserve(basename.size());

    for(wchar_t c:basename){
    //如果有找到全形或特別的空白
    if(c == L'\u3000' || c == L'\u00A0' || c == L'\u2002' || c == L'\u2003' ||
        c == L'\u2009' || c == L'\u202F' || c == L'\u205F' || c == L'\u200B')
    {
        result.push_back(L' ');
    }else if(c == L' '){
        result.push_back(L' ');
    }else if(c == L'<' || c == L'>' || c == L':' || c == L'"' ||
                c == L'/' || c == L'\\' || c == L'|' || c == L'?' || c == L'*') // 防止非法檔案名稱
    {
        result.push_back(L'_');
    }else{
        result.push_back(c);
    }
    }

    // 去除開頭或結尾空白（避免 Windows 檔名非法）
    while (!result.empty() && std::iswspace(result.front())) result.erase(result.begin());
    while (!result.empty() && std::iswspace(result.back()))  result.pop_back();

    //如果有對檔案名稱進行改動,傳入資料流
    if(result != basename){
    Console::ensureWcerr(L"[Notice] 檔名中包含不安全字元，已自動修正。\n");
    }

    return result;
  } 
  
  // 偵測結果的錯誤處理
  DetectorCategory handleDetectorResult(DetectorResult r,ErrorHandle& errorhandler){
    switch(r){
        case DetectorResult::isOK :
        case DetectorResult::Invalid:
        return DetectorCategory::OK;
        
        case DetectorResult:: NotRTF:
        {
        ErrorSystem::ErrorInfo info;
        info.add(ErrorSystem::ErrorType::NotRTF,
                ErrorSystem::ErrorLevel::FatalLocal,
                ErrorSystem::ErrorCategory::DetectEncoding,
                L"偵測出非目標 RTF 檔,結束處理當下程式");
        errorhandler.handle(info);
        return DetectorCategory::Fatal;
        }
        
        case DetectorResult::HeaderMalformed :
        {
        ErrorSystem::ErrorInfo info;
        info.add(ErrorSystem::ErrorType::Detector_Fatal,
                ErrorSystem::ErrorLevel::FatalLocal,
                ErrorSystem::ErrorCategory::DetectEncoding,
                L"檔案標頭格式不符合規定:{\\rtfN");
        errorhandler.handle(info);
        return DetectorCategory::Fatal;
        }
        
        case DetectorResult::UnsupportedEncoding :
        {
        ErrorSystem::ErrorInfo info;
        info.add(ErrorSystem::ErrorType::Detector_Fatal,
                ErrorSystem::ErrorLevel::FatalLocal,
                ErrorSystem::ErrorCategory::DetectEncoding,
                L"檔案並非支援的處理編碼");
        errorhandler.handle(info);
        return DetectorCategory::Fatal;
        }

        case DetectorResult::EncodingConflict :
        {
        ErrorSystem::ErrorInfo info;
        info.add(ErrorSystem::ErrorType::Detector_Warning,
                ErrorSystem::ErrorLevel::Warning,
                ErrorSystem::ErrorCategory::DetectEncoding,
                L"[警告]: 顯示格式有矛盾");
        errorhandler.handle(info);
        return DetectorCategory::Warning;
        }

        case DetectorResult::CannotDetect :
        {
        ErrorSystem::ErrorInfo info;
        info.add(ErrorSystem::ErrorType::Detector_Recoverable,
                ErrorSystem::ErrorLevel::Recoverable,
                ErrorSystem::ErrorCategory::DetectEncoding,
                L"無法正確辨別檔案編碼格式");
        errorhandler.handle(info);
        return DetectorCategory::Recoverable;
        }

        case DetectorResult::EmptyFile:
        {
        ErrorSystem::ErrorInfo info;
        info.add(ErrorSystem::ErrorType::Detector_Warning,
                ErrorSystem::ErrorLevel::Warning,
                ErrorSystem::ErrorCategory::DetectEncoding,
                L"[警告]: 目標檔案內容為空");
        errorhandler.handle(info);
        return DetectorCategory::Warning;
        }

        case DetectorResult::FileTooSmall :
        {
        ErrorSystem::ErrorInfo info;
        info.add(ErrorSystem::ErrorType::Detector_Fatal,
                ErrorSystem::ErrorLevel::FatalLocal,
                ErrorSystem::ErrorCategory::DetectEncoding,
                L"目標檔案內容過少,不可能以預期格式出現");
        errorhandler.handle(info);
        return DetectorCategory::Fatal;
        }

        case DetectorResult::BinaryFile :
        {
        ErrorSystem::ErrorInfo info;
        info.add(ErrorSystem::ErrorType::Detector_Fatal,
                ErrorSystem::ErrorLevel::FatalLocal,
                ErrorSystem::ErrorCategory::DetectEncoding,
                L"目標並非文字檔,無法處理");
        errorhandler.handle(info);
        return DetectorCategory::Fatal;
        }

        case DetectorResult::WrongStep :
        {
        ErrorSystem::ErrorInfo info;
        info.add(ErrorSystem::ErrorType::Detector_Warning,
                ErrorSystem::ErrorLevel::Warning,
                ErrorSystem::ErrorCategory::DetectEncoding,
                L"程式執行流程錯誤");
        errorhandler.handle(info);
        return DetectorCategory::InternalError;
        }
        default:
        break;
    }
    return DetectorCategory::Warning;
  }

  // 用來確保輸入的路徑 以 utf-8 的 std::string 呈現
  std::string pathToUtf8(const std::filesystem::path& p) {
    #ifdef _WIN32
    // Windows：filesystem::path 內部是 UTF-16
    const std::wstring& w = p.wstring();

    if (w.empty()) {
        return std::string{};
    }

    // UTF-16 → UTF-8
    int sizeNeeded = WideCharToMultiByte(
        CP_UTF8,
        0,
        w.data(),
        static_cast<int>(w.size()),
        nullptr,
        0,
        nullptr,
        nullptr
    );

    if (sizeNeeded <= 0) {
        return std::string{};
    }

    std::string result(sizeNeeded, '\0');

    WideCharToMultiByte(
        CP_UTF8,
        0,
        w.data(),
        static_cast<int>(w.size()),
        result.data(),
        sizeNeeded,
        nullptr,
        nullptr
    );

    return result;

    #else
    // Linux / macOS：path 本來就假設是 UTF-8
    return p.string();
    #endif
  }

  // 錯誤處理enum
  enum class HandleDecision{
    Continue,
    SkipFile,
    TerminateAll
  };

  // 用來處裡所有回傳的 ErrorInfo 
  class ErrorInfoProcessor{
  public:
    HandleDecision process(const ErrorSystem::ErrorInfo& info,ErrorHandle& handler){
        if(info.isEmpty()){
        return HandleDecision::Continue;
        }

        handler.handle(info);

        switch(info.getMaxLevel()){
        case ErrorSystem::ErrorLevel::None:
            return HandleDecision::Continue;
        case ErrorSystem::ErrorLevel::Warning:
            return HandleDecision::Continue;
        case ErrorSystem::ErrorLevel::Recoverable:
            return HandleDecision::SkipFile;
        case ErrorSystem::ErrorLevel::FatalLocal:
            return HandleDecision::SkipFile;
        case ErrorSystem::ErrorLevel::FatalGlobal:
            return HandleDecision::TerminateAll;
        }

        return HandleDecision::Continue;
    }  
  };

  // 專門用來存放所有需要資訊的類別
  class FileContext{
    DetectOutput detectOut;
    int codepage_ = 0;
    std::filesystem::path inputPath_;
    std::filesystem::path outputDir_;
  public:
    enum class EncodingType{
            UTF8,
            UTF16,
            ANSI
    };
    
    void setOutPut(DetectOutput input){
        detectOut = input;  
        switch(detectOut.encoding){
            case Encoding::Ansi_Big5 :
            codepage_ = 950;
            break;
            case Encoding::Ansi_GBK :
            codepage_ = 936;
            break;
            case Encoding::Ansi_JIS :
            codepage_ = 932;
            break;
            case Encoding::Ansi_Korean :
            codepage_ = 949;
            break;
            case Encoding::Ansi_Latin_1 :
            codepage_ = 1252;
            break;
            case Encoding::Ansi_CEI :
            codepage_ = 1250;
            break;
            case Encoding::Ansi_Cyrillic :
            codepage_ = 1251;
            break;
            default:
            break;
        }
    }
        
    Encoding getEncoding() const{
        return detectOut.encoding;
    }

    DetectorResult getDetectResult() const{
        return detectOut.result;
    }

    DetectOutput getDetectOutput() const{
        return detectOut;
    }

    int getCodepage(){
        return codepage_;
    }
        
    EncodingType getEncodingType(){
        switch(detectOut.encoding){
        case Encoding::UTF8_BOM : return EncodingType::UTF8;
        case Encoding::UTF8_NoBOM : return EncodingType::UTF8;
        case Encoding::UTF16_BE : return EncodingType::UTF16;
        case Encoding::UTF16_LE : return EncodingType::UTF16;
        case Encoding::Ansi_Big5 : return EncodingType::ANSI;
        case Encoding::Ansi_CEI : return EncodingType::ANSI;
        case Encoding::Ansi_Cyrillic : return EncodingType::ANSI;
        case Encoding::Ansi_GBK : return EncodingType::ANSI;
        case Encoding::Ansi_JIS : return EncodingType::ANSI;
        case Encoding::Ansi_Korean : return EncodingType::ANSI;
        case Encoding::Ansi_Latin_1 : return EncodingType::ANSI;
        default: break;
        }
        
        return EncodingType::UTF8;
    }  

    void setFilePath(const std::filesystem::path& filepath){
        inputPath_ = filepath;
    }

    const std::filesystem::path& getFilePath() const noexcept{
        return inputPath_;
    }

    void setOutputDir(const std::filesystem::path& dir){
        outputDir_ = dir;
    }

    const std::filesystem::path& getOutputDir() const noexcept{
        return outputDir_;
    }
  };

}

// 主流程函式定義
bool RTFProcessor::processFile(const std::filesystem::path& filePath,
                               const std::filesystem::path& outputpath,
                               Common::OutputFormat outputformat,
                               ProcessMode mode,
                               std::optional<std::filesystem::path> taskRootDir)
{
    // 擷取不含副檔名的檔案名稱
    std::wstring baseName = filePath.stem().wstring();
    // 檢查與替換檔名中會造成問題的空格
    baseName = sanitizeFileName(baseName);
    if(baseName.empty()){ // 防止特殊檔名導致空白檔名
      baseName = L"output";
    }
    std::filesystem::path outputSet = outputpath / baseName;
    // 建立輸出資料夾
    OutputDirGuard fileOut(outputSet);
    auto dirResult = fileOut.ensure();
    if(dirResult != EnsureDirResult::Success){
      switch(dirResult){
        case EnsureDirResult::AlreadyExists:
        Console::ensureWcerr(L"輸出資料夾已存在: " + outputSet.wstring() + L"\n");
        break;
        case EnsureDirResult::NotDirectory:
        Console::ensureWcerr(L"輸出路徑已存在但不是資料夾: " + outputSet.wstring() + L"\n");
        break;
        case EnsureDirResult::CreateFailed:
        Console::ensureWcerr(L"建立輸出資料夾失敗: " + outputSet.wstring() + L"\n");
        break;
        case EnsureDirResult::VerifyFailed:
        Console::ensureWcerr(L"建立後驗證輸出資料夾失敗: " + outputSet.wstring() + L"\n");
        break;
        default:
        Console::ensureWcerr(L"未知的輸出資料夾錯誤: " + outputSet.wstring() + L"\n");
        break;
      }
      return false;
    }
    
    logSystem logger;
    
    if(!logger.open(fileOut.path())){
      std::wcout<< L"測試失敗,開啟有問題\n";
      return false; 
    }
    logger.log(LogLevel::Info,"以確保有輸出資料夾");

    Console::ensureWcout(L"正在處理檔案: " + filePath.wstring() + L"\n");
    logger.log(LogLevel::Info,std::string("正在處理檔案: ") + pathToUtf8(filePath));

    ErrorHandle errorhandler(logger);
    errorhandler.beginFile();
    FileScopeGuard guard{errorhandler}; // 使用類別的解構功能自動呼叫endfile()
    logger.log(LogLevel::Info,"錯誤系統啟用");
    
    logger.log(LogLevel::Info,"嘗試開啟輸入檔案");
    // 開啟輸入檔案（用 binary 避免編碼問題）
     std::ifstream input(filePath, std::ios::binary);
     if (!input) {
      ErrorSystem::ErrorInfo info;
      info.add(ErrorSystem::ErrorType::Fatal_FileOpenFail,
               ErrorSystem::ErrorLevel::FatalLocal,
               ErrorSystem::ErrorCategory::FileIO,
               L"無法正確開啟目標檔案 : " + filePath.wstring());
      errorhandler.handle(info);
      return false;
     }
     logger.log(LogLevel::Info,"成功開啟輸入檔案");
     
     // 將輸入檔案讀成 string 好做之後的加工
    std::string rtfContent{
      (std::istreambuf_iterator<char>(input)),
       std::istreambuf_iterator<char>()
    };

    //偵測檔案,並將得到的資訊放入指定類別中
    //判斷檔案是用何種基礎編碼,並順便收集該基礎編碼所屬的資訊
    EncodingDetector detect;
    FileContext filecontext;
    filecontext.setOutPut(detect.detectEncoding(input)); // 將得到到資料放入資料存放的類別中
    logger.log(LogLevel::Info,"對輸入檔案進行偵測");
     
    //偵測結果錯誤處理 
    auto result = filecontext.getDetectResult();
    auto cat = handleDetectorResult(result,errorhandler);
     
    switch(cat){
      case DetectorCategory::Recoverable:
      case DetectorCategory::Fatal:
      logger.log(LogLevel::Error,"輸入檔案偵測未通過");
      return false;
      default: break;
    }
    logger.log(LogLevel::Info,"輸入檔案偵測成功");
     
    // 將目標檔案的資料放進專們的類別中
    filecontext.setFilePath(filePath); // 給定輸入路徑
     
    // 圖片區況處理
    PictDisassembler pictdisassemcler(errorhandler,fileOut);
    auto picrResult = pictdisassemcler.process(rtfContent,logger);
    switch(picrResult){
      case PictProcessResult::OK :
      logger.log(LogLevel::Info,"圖片區塊正確結束");
      break;
      case PictProcessResult::SkipPict :
      logger.log(LogLevel::Warn,"圖片區塊有部份進行錯誤區塊跳過處理,但是可繼續後面流程");
      break;
      case PictProcessResult::AbortFile:
      logger.log(LogLevel::Error,"圖片區塊有無法跳過處裡之錯誤!,終止後續流程");
      return false;
      case PictProcessResult::AbortGlobal:
      logger.log(LogLevel::Error,"圖片區塊有破壞性之錯誤!,終止整個程序!");
      errorhandler.handleFatalGlobal(L"圖片區塊破壞性錯誤導致整體程式緊急終止!!!");
    }
  
    // 如果偵測結果是 utf16 就紀錄 log 並將其當作 utf8 處裡
    if(filecontext.getEncodingType() == FileContext::EncodingType::UTF16){
      logger.log(LogLevel::Warn,"編碼偵測為: UTF-16 非RTF支援之編碼類型嘗試改走utf-8的路線");
    }
    
    logger.log(LogLevel::Info,"依照編碼體系進行 解碼 與 群組處理 並檢查其是否有錯誤");
    // 依照不同的編碼方式正確的將 ifstream 複製的陣列轉換成 std::string 
    switch(filecontext.getEncodingType()){
      case FileContext::EncodingType::UTF8 :
      case FileContext::EncodingType::UTF16:
      {
       Utf8ErrorDetector utf_detector;
       auto utf_decoded = utf8Decoder().decode(rtfContent,utf_detector,errorhandler);
       if(!utf_decoded) return false;
       rtfContent = utf_decoded.value();
       utf8GroupProcessor().processGroups(rtfContent);//加入處理群組的功能
       Console::ensureWcout(L"是UTF編碼體系的檔案\n");
       logger.log(LogLevel::Info,"完成 UTF 體系的解碼與群組處理");
       break;
      }
      case FileContext::EncodingType::ANSI :
      {
       AnsiErrorDetector detector(filecontext.getEncoding());
       ansiDecoder ansiDe;
       auto ansi_decoded = ansiDe.setCodePage(filecontext.getCodepage()).decode(rtfContent,detector,errorhandler); // 使用串聯特性
       if(!ansi_decoded) return false; // 偵測結果錯誤就中斷函式執行
       rtfContent = ansi_decoded.value();
       Console::ensureWcout(L"是ANSI編碼的檔案\n");
       logger.log(LogLevel::Info,"完成 ANSI 體系的解碼");
       break;
      }
    }
    
    logger.log(LogLevel::Info,"開始檢查檔案是否符合 RTF 文法");
    //檢查 圖檔區域處裡完的純文字是否符合 rtf　文法,並將結果放給統一函式處裡 
    auto GeneralInfo = GeneralErrorDetector().detect(rtfContent);
    auto pro = ErrorInfoProcessor().process(GeneralInfo,errorhandler);
    switch(pro){
      case HandleDecision::Continue:
      logger.log(LogLevel::Info,"輸入檔案的 RTF文法檢查通過");
      break;
      case HandleDecision::SkipFile:
      case HandleDecision::TerminateAll:
      Console::ensureWcerr(L"[圖片處理流程後的文法檢查階段出現致命錯誤]: " + filePath.wstring());
      logger.log(LogLevel::Error,"輸入檔案不符合 RTF 之文法");
      return false;
    }
    
    logger.log(LogLevel::Info,"清理轉換後之字串剩餘之控制符");
    //用類別的功能來清除文字中剩餘的控制碼等等
    textRtfProcessor().Processor(rtfContent,logger);
    

    logger.log(LogLevel::Info,"嘗試開啟輸出檔案");
    
    
    // 開啟輸出檔案
    std::wstring ext;
    switch(outputformat){
        case Common::OutputFormat::Txt:  ext = L".txt"; break;
        case Common::OutputFormat::Md:   ext = L".md"; break;
        case Common::OutputFormat::Html: ext = L".html"; break;
        default:
        Console::ensureWcerr(L"[Unknown OutputFormat]");
        return false;
    }
    
    std::filesystem::path textOutput = fileOut.path() / (baseName + ext);
    std::ofstream output(textOutput, std::ios::binary);
    if (!output) {
      ErrorSystem::ErrorInfo info;
      info.add(ErrorSystem::ErrorType::Fatal_CreatOutputFail,
               ErrorSystem::ErrorLevel::FatalLocal,
               ErrorSystem::ErrorCategory::FileIO,
               L"無法建立輸出檔案 : " + textOutput.wstring());
      errorhandler.handle(info);
      return false;
    }
    logger.log(LogLevel::Info,"輸出檔案開啟成功");

    // .txt 檔案寫入 UTF-8 BOM 讓 Windows 筆記本正常顯示
    if(outputformat == Common::OutputFormat::Txt){
      const unsigned char bom[] = {0xEF, 0xBB, 0xBF};
      output.write(reinterpret_cast<const char*>(bom), 3);
    }
    
    DocumentBuilder DocBuilder;
    Document Doc = DocBuilder.build(rtfContent);
    logger.log(LogLevel::Info,"將處裡好的字串拆解為語意模型");

    switch(outputformat){
      case  Common::OutputFormat::Txt:{
        TxtRenderer txtRen;
        txtRen.render(Doc,output);
        logger.log(LogLevel::Info,"將語意模型以指定之檔案類型(txt)輸出");
        break;
      }
      
      case Common::OutputFormat::Md:{
        MarkdownRenderer mdRen;
        mdRen.render(Doc,output);
        logger.log(LogLevel::Info,"將語意模型以指定之檔案類型(md)輸出");
        break;
      }
      
      case Common::OutputFormat::Html:{
        HtmlRenderer htmlren;
        htmlren.render(Doc,output);
        logger.log(LogLevel::Info,"將語意模型以指定之檔案類型(html)輸出");
        break;
      }
      
      default:
      Console::ensureWcerr(L"[Unknown OutputFormat]");
      return false;
    }
    
    Console::ensureWcout(L"已輸出至: " + baseName + L"\n");
    output.close();
    logger.log(LogLevel::Info,"關閉輸出檔案");
    
    logger.close();
    return true;
}
