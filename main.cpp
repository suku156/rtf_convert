#pragma execution_character_set("utf-8")
#include<iostream>
#include<fstream>
#include<string>
#include<sstream>
#include<vector>
#include<filesystem>
#include<fcntl.h>
#include<iconv.h>      
#include<algorithm>
#include<cstdlib>
#include<cctype>
#include<cstdint>
#include<optional>
#include<iomanip>
#include<thread>
#include<atomic>
#include<mutex>
#include<cstring>
#include "ErrorSystem_Module/ErrorSystem.h" // 放置錯誤系統資訊
#include "LogSystem_Module/LogSystem.h"   // 日誌系統的模組
#include "Universal_Module/CommonEnum.h"  // 多個模組共用的 enum
#include "FormatDetection_Module/RtfFormatDetection.h" // 偵測並判斷 rtf 格式與基礎編碼的模組 
#include "ErrorDetector_Module/ErrorDetector.h" // 錯誤偵測的模組
#include "ErrorSystem_Module/ErrorHandle.h"   // 決定如何處裡錯誤的模組
#include "EncodingProcess_Module/Decode.h"        // 解碼模組
#include "EncodingProcess_Module/GroupProcessor.h" // 處裡 utf 體系群組的模組
#include "Universal_Module/Console.h"        // 用來確保多執行緒的情況下 std::wcout 以及 std::wcerr 不會互相交叉
#include "TextProcessor_Module/textProcessor.h"  // 用來處理解碼完的文字
#include "Universal_Module/OutputDirGuard.h" // 用來確保輸出資料夾的模組
#include "PictProcessor_Module/pictProcessor.h"  // 處裡圖片的模組
#include "SemanticStructure_Module/Document.h"       // 語意結構與轉化成語意結構用的模組
#include "SemanticStructure_Module/Renderer.h"       // 依據需求解讀語意結構用的模組
#include "Cli_Module\CliParser.h"        // Cli 命令列選項模組

#ifdef _WIN32
  #include <windows.h>
#endif

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

// 用來區分處理的是 單獨檔案 還是 資料夾
enum class ProcessMode{SingleFile,BatchFile};

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
    
  }
  return DetectorCategory::Warning;
}

// 用來接收進度資訊並決定如何呈現的類別
class ProgressObserver{
  size_t total_{};
  std::atomic_size_t done_{0};
public:
  void Start(size_t num){
    total_ = num;
    done_.store(0,std::memory_order_relaxed);
    display();
  }

  void onUnitDone(){
    done_.fetch_add(1,std::memory_order_relaxed);
    display();
  }

  void Finish(){
    display();
    Console::ensureWcout( L" 已完成全部任務!\n");
  }

private:

  void display() const{
    Console::ensureWcout(L"已完成 " +
                std::to_wstring(done_.load(std::memory_order_relaxed)) +
                L" / 總數: " +
                std::to_wstring(total_));
  }
  
};

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

// 主要執行流程
// 目標為資料夾時(ProcessMode::BatchFile) 需要額外傳入原本的目標資料夾路徑當作第四個參數
class RTFProcessor{
public:
   // 用來處理目標
   void processFile(const std::filesystem::path& filePath,
                    ProcessMode mode,
                    std::optional<std::filesystem::path> taskRootDir = std::nullopt)
    {
    // 擷取不含副檔名的檔案名稱
    std::wstring baseName = filePath.stem().wstring();
    // 檢查與替換檔名中會造成問題的空格
    baseName = sanitizeFileName(baseName);
    std::filesystem::path outputDir = filePath.parent_path() / baseName;
    OutputDirGuard fileOut(outputDir);
    //確保有輸出資料夾
    if(!fileOut.ensure()){
      Console::ensureWcerr(L"[FatalLocal] 沒有可用的輸出資料夾: " + filePath.wstring());
      return;
    }
    
    logSystem logger;
    
    if(!logger.open(fileOut.path())){
      std::wcout<< L"測試失敗,開啟有問題\n";
      return; 
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
      return;
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
      return;
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
      return;
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
       if(!utf_decoded) return;
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
       if(!ansi_decoded) return; // 偵測結果錯誤就中斷函式執行
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
      return;
    }
    
    logger.log(LogLevel::Info,"清理轉換後之字串剩餘之控制符");
    //用類別的功能來清除文字中剩餘的控制碼等等
    textRtfProcessor().Processor(rtfContent,logger);
    

    logger.log(LogLevel::Info,"嘗試開啟輸出檔案");
    // 開啟輸出檔案
    // 將 std::wstring 透過 .c_str() 轉換為 const wchar_t*
    std::filesystem::path textOutput = fileOut.path() / (baseName+L".txt");
    std::ofstream output(textOutput, std::ios::binary);
    if (!output) {
      ErrorSystem::ErrorInfo info;
      info.add(ErrorSystem::ErrorType::Fatal_CreatOutputFail,
               ErrorSystem::ErrorLevel::FatalLocal,
               ErrorSystem::ErrorCategory::FileIO,
               L"無法建立輸出檔案 : " + textOutput.wstring());
      errorhandler.handle(info);
      return ;
    }
    logger.log(LogLevel::Info,"輸出檔案開啟成功");

    // 寫入 UTF-8 BOM 讓 Windows 筆記本正常顯示
    const unsigned char bom[] = {0xEF, 0xBB, 0xBF};
    output.write(reinterpret_cast<const char*>(bom), 3);
    
    
    DocumentBuilder DocBuilder;
    Document Doc = DocBuilder.build(rtfContent);
    logger.log(LogLevel::Info,"將處裡好的字串拆解為語意模型");

    TxtRenderer txtRen;
    txtRen.render(Doc,output);
    logger.log(LogLevel::Info,"將語意模型以預設之檔案類型輸出");
    
    Console::ensureWcout(L"已輸出至: " + baseName + L"\n");
    output.close();
    logger.log(LogLevel::Info,"關閉輸出檔案");
    
    logger.close();
  }
};

// 多執行緒任務的總管
class RTFDirectoryRunner{
public:
  void run(const std::filesystem::path& dirPath,ProgressObserver& ProOB){
    // 1.建立工作清單
    std::vector<std::filesystem::path> files;
    files = collectRtfFiles(dirPath);
    if(files.empty()) return;
    
    // 設定進度條的總數
    ProOB.Start(files.size());

    // 2.決定執行緒數量
    size_t threadNum = DecideThreadNum(files.size());
    if(threadNum == 0) return;//防呆

    // 3. atomic index 任務分配器 確保各執行緒之間不會因隨機執行搶任務
    std::atomic_size_t index{0};

    // 4. 用 lambda 執行某個任務
    auto worker = [&](){
      RTFProcessor  localprocessor;
      
      while(true){
        size_t i = index.fetch_add(1);
        if(i >= files.size()) break;

        const auto& file = files[i];

        try{
          localprocessor.processFile(file,ProcessMode::BatchFile,dirPath);
        }catch(const std::exception& e){
          Console::ensureWcerr(L"[Exception] " + file.wstring() + L"\n ");
        }
        catch(...){
          Console::ensureWcerr(L"[Unknown Exception] " + file.wstring() +  L"\n");
        }
        
        ProOB.onUnitDone();
      }
    };

    // 5.建立 thread
    std::vector<std::thread> threads;
    threads.reserve(threadNum);

    for(size_t t =0; t < threadNum;t++){
      threads.emplace_back(worker);
    }

    // 6.確保所有 thread 結束
    for(auto& th:threads){
      th.join();
    }

    ProOB.Finish();
  }
  
private:
  std::vector<std::filesystem::path> collectRtfFiles(const std::filesystem::path& dirPath){
    std::vector<std::filesystem::path> files;
    for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
      if (entry.is_regular_file() && entry.path().extension() == L".rtf") {
        files.push_back(entry.path());
      }
    }
    return files;
  }

  size_t DecideThreadNum(size_t resultCount){
    if(resultCount == 0) return 0;

    size_t systemMax = std::thread::hardware_concurrency();
    if(systemMax == 0){ // 防呆
      systemMax = 4;
    }

    systemMax = std::min(systemMax,size_t(16)); // 最多就開十六條執行緒

    return std::min(systemMax,resultCount);
  }
};

int wmain(int argc,wchar_t* argv[]){
    // 強制讓 wcout 用 UTF-16 (Windows 本地寬字輸出)
    _setmode(_fileno(stdout), _O_U16TEXT);
    // 強制讓 wcerr 用 UTF-16 (Windows 本地寬字輸出)
    _setmode(_fileno(stderr), _O_U16TEXT);

    //主程序的日誌系統
    logSystem mainlogger;
    // 錯誤處理系統變數建立
    ErrorHandle mainerrorhandler(mainlogger);
    
    // 簡易防呆
    if(argc < 2){
      mainerrorhandler.handleFatalGlobal(L"需要正確輸入: "+std::wstring(argv[0])+ L" <檔案/資料夾路徑>\n");
    }

    // 命令列紀錄
    Cli::ParseResult parseresult = Cli::parse(argc,argv);
    if(!parseresult.ok){
      Console::ensureWcout(parseresult.message);
      return 2;
    }

    //使用類別來執行任務
    RTFProcessor processor;
    std::filesystem::path filePath(argv[1]); // 直接使用 wchar_t 接收路徑
    
    if (!std::filesystem::exists(filePath)) {
      mainerrorhandler.handleFatalGlobal(L"找不到檔案/資料夾: "+ filePath.wstring() + L"\n");
    }

    // 分辨目標,依照目標屬性呼叫相對的函式
    
    if(std::filesystem::is_regular_file(filePath)){
      processor.processFile(filePath,ProcessMode::SingleFile);
    }else if(std::filesystem::is_directory(filePath)){
      RTFDirectoryRunner Drunner;
      ProgressObserver ProOB;
      Drunner.run(filePath,ProOB);
    }
    
    return EXIT_SUCCESS;
}
