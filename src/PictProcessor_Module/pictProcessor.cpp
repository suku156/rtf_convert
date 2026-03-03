#include "pictProcessor.h"
#include "ErrorSystem_Module/ErrorHandle.h"
#include "Universal_Module/OutputDirGuard.h"
#include "LogSystem_Module/LogSystem.h"
#include "Universal_Module/Console.h"
#include "ErrorSystem_Module/ErrorSystem.h"
#include<string>
#include<cstddef>
#include<cstdint>
#include<vector>
#include<filesystem>
#include<optional>
#include<string_view>
#include<iostream>
#include<cctype>
#ifdef _WIN32
  #include <windows.h>
#endif

// 呼叫外部程式 magic.exe 執行將 wmf 圖片轉換成 png 的任務
int ProcessRunner::run(const std::filesystem::path& exe,
                 const std::vector<std::filesystem::path>& args)
  {
  #ifdef _WIN32
    // 第一步 : 將輸入的寬字元 轉化為 符合格式的寬字元陣列儲存
    std::wstring cmd;
    cmd.reserve(1024);

    cmd += quoterArgW(exe.wstring());
    for(const auto& a:args){
      cmd += L" ";
      cmd += quoterArgW(a.wstring());
    }

    // 第二步 : 準備轉換需要的參數
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};

    std::vector<wchar_t> buffer(cmd.begin(),cmd.end());
    buffer.push_back(L'\0');

    // 第三步 : 使用 windows 底層建立 API 的程序 來使用 magic.exe
    BOOL ok = CreateProcessW(
      nullptr,
      buffer.data(),  // command line
      nullptr, nullptr,
      FALSE,
      CREATE_NO_WINDOW, // no console window
      nullptr,
      nullptr,
      &si,
      &pi
    );

    //  檢查是否成功
    if(!ok){
      return -1;
    }
    
    // 等待外部程序(magic.exe) 跑完
    WaitForSingleObject(pi.hProcess,INFINITE);

    // 取的外部程序的回傳結束碼(用於觀察是否成功執行)
    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    // 正確的釋放 windows 的資源
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return static_cast<int>(exitCode);
  
  #else
    // 非 windows 系統的處理
     std::string cmd;
    cmd.reserve(1024);

    cmd += quoteArgA(exe.string());
    for (const auto& a : args) {
        cmd += " ";
        cmd += quoteArgA(a.string());
    }

    int rc = std::system(cmd.c_str());
    return rc;
  #endif  
}
std::wstring ProcessRunner::quoterArgW(const std::wstring& s){
    std::wstring out = L"\"";
    for(wchar_t ch : s){
      if(ch == L'"'){
        out += L"\\\"";
      }else{
        out += ch;
      }
    }
    out += L"\"";
    return out;
}
std::string  quoteArgA(const std::string& s){
   std::string out = "\"";
    for (char ch : s) {
      if(ch == '"'){
        out += "\\\"";
      }else{
        out += ch;
      } 
    }
    out += "\"";
    return out;
}

// 將獲得的 byte 資料流轉換成圖片
bool imageProcessor::saveRaw(const PictInfo& info) const{
    if(info.bytes.empty()) return false;
    if(info.outputPath.empty()) return false;

    std::ofstream out(info.outputPath,std::ios::binary);
    if(!out) return false;
    
    out.write(reinterpret_cast<const char*>(info.bytes.data()), info.bytes.size());
    return out.good();
  }
bool imageProcessor::convertToPng(const std::filesystem::path& inWmfOrEmf,
                    const std::filesystem::path& outPng) const
{
    namespace fs = std::filesystem;

    // ---------- 基本檢查 ----------
    if(inWmfOrEmf.empty() || outPng.empty()) return false;
    if(!fs::exists(inWmfOrEmf)) return false;

    // 只允許 WMF/EMF
    auto ext = inWmfOrEmf.extension().wstring();
    for(auto& ch : ext) ch = (wchar_t)towlower(ch);
    if(ext != L".wmf" && ext != L".emf"){
      return false;
    }

     // 建立輸出資料夾
    fs::path outDir = outPng.parent_path();
    if(outDir.empty()) return false;

    std::error_code ec;
    fs::create_directories(outDir, ec);
    if(ec) return false;

    // 清掉舊檔避免誤判
    fs::remove(outPng, ec);
  #ifdef _WIN32
    fs::path exe = L"magick";
  #else
    fs::path exe = "magick";
  #endif
    
    int exitCode = ProcessRunner::run(exe,{inWmfOrEmf,outPng});
    if(exitCode != 0) return false;
    // 確認 PNG 產生且大小 > 0
    if (!fs::exists(outPng)) return false;
    if (fs::file_size(outPng, ec) == 0 || ec) return false;

    return true;
}
bool imageProcessor::process(const PictInfo& info, bool wantPng) const {
    // 先輸出 raw（最保守、一定可用）
    if (!saveRaw(info)) return false;

    // 若想轉 png（只針對 WMF/EMF）
    if (wantPng && (info.format == PictFormat::WMF || info.format == PictFormat::EMF)) {
        Console::ensureWcout(L"有觸發轉換程序\n");
        std::filesystem::path outPng = info.outputPath;
        outPng.replace_extension(".png");
        bool ok = convertToPng(info.outputPath, outPng);
        if(!ok){
          Console::ensureWcerr(L"選取的額外圖片轉換功能(wmf/emd -> png) 轉換失敗!\n");
        }
    }

    return true;
}
std::string imageProcessor::quotePath(const std::filesystem::path& p){
    return "\"" + p.string() + "\"";
}

// 圖片處理的總控
PictProcessResult PictDisassembler::process(std::string& rtfContent,logSystem& logger){
    std::vector<ReplaceTask> tasks;
    logger.log(LogLevel::Info,"偵測檔案中是否有圖片區塊需要處裡");
    size_t pos = rtfContent.find("{\\pict");
    if(pos == std::string::npos){
      logger.log(LogLevel::Info,"輸入檔案中未偵測到圖片區塊");
      return PictProcessResult::OK; // 找不到直接結束
    } 
    ErrorSystem::ErrorInfo PictErrorInfo;

    logger.log(LogLevel::Info,"偵測到圖片區塊,圖片處理開始");
    
    while (pos != std::string::npos && pos < rtfContent.size()) {
      
      size_t start = pos; // 設定開頭位置
      
      auto ScanReport = scanGroup(rtfContent,start);
      if(!ScanReport.braceBalanced){ // 大括號不平衡
        std::wstring ws = L"第" + std::to_wstring(pictCount_);
        PictErrorInfo.add(ErrorSystem::ErrorType::Picture_StructureError,
                          ErrorSystem::ErrorLevel::FatalLocal,
                          ErrorSystem::ErrorCategory::Picture,
                          (ws + L"圖片區塊群組括號數量不平衡!"));
        errorHandler_.handle(PictErrorInfo);
        return PictProcessResult::AbortFile;
      }
      size_t end = ScanReport.groupEnd;
      if(end > rtfContent.size()){
        std::wstring ws = L"第" + std::to_wstring(pictCount_);
        PictErrorInfo.add(ErrorSystem::ErrorType::Picture_StructureError,
                          ErrorSystem::ErrorLevel::FatalLocal,
                          ErrorSystem::ErrorCategory::Picture,
                          (ws + L"找到的圖片結尾位置超過檔案大小!!"));
        errorHandler_.handle(PictErrorInfo);
        return PictProcessResult::AbortFile;
      }
      if(ScanReport.pictHexInterrupted){ // 圖片 hex 區中間有控制符
        std::wstring ws = L"第" + std::to_wstring(pictCount_);
        PictErrorInfo.add(ErrorSystem::ErrorType::Picture_Hex_Broken,
                          ErrorSystem::ErrorLevel::Recoverable,
                          ErrorSystem::ErrorCategory::Picture,
                          (ws + L"圖片的 Hex 區塊含有控制符視為損毀,進行跳過處裡"));
        tasks.push_back({start,end,"[[IMG_ERR_" + std::to_string(pictCount_) + "]]"});
        pictCount_++;
        // 找到下一個的開頭
        pos = rtfContent.find("{\\pict",end);
        continue;
      }
      if(ScanReport.detectedNewGroupStart){ // 圖片 hex 區中間有新的群組(不合規定)
        std::wstring ws = L"第" + std::to_wstring(pictCount_);
        PictErrorInfo.add(ErrorSystem::ErrorType::Picture_Hex_Broken,
                          ErrorSystem::ErrorLevel::Recoverable,
                          ErrorSystem::ErrorCategory::Picture,
                          (ws + L"圖片的 Hex 區塊含有新的群組視為損毀,進行跳過處裡"));
        tasks.push_back({start,end,"[[IMG_ERR_" + std::to_string(pictCount_) + "]]"});
        pictCount_++;
        // 找到下一個的開頭
        pos = rtfContent.find("{\\pict",end);
        continue;
      }

      std::string_view groupView(rtfContent.data() + start , end -start);
      size_t hexstart = findPictHexStart(groupView,0);
     
      if(hexstart == std::string::npos){
        std::wstring ws = L"第" + std::to_wstring(pictCount_);
        PictErrorInfo.add(ErrorSystem::ErrorType::Picture_Retrieve_Data_Failed,
                          ErrorSystem::ErrorLevel::Recoverable,
                          ErrorSystem::ErrorCategory::Picture,
                          (ws + L"圖片區塊中無法定位 header 與 hex 資料的分界!"));
        tasks.push_back({start,end,"[[IMG_ERR_" + std::to_string(pictCount_) + "]]"});
        std::string logstr = std::string("第 ") + std::to_string(pictCount_) + 
                             std::string(" 圖片區塊無法區分控制邊界,跳過處理");
        logger.log(LogLevel::Warn,logstr);
        pictCount_++;
        // 找到下一個的開頭
        pos = rtfContent.find("{\\pict",end);
        continue;
      }
      
      std::string_view hexScope(groupView.data() + hexstart , end - hexstart - 1);
      if(isWmfHexTooShort(hexScope)){
        std::wstring ws = L"第" + std::to_wstring(pictCount_);
        PictErrorInfo.add(ErrorSystem::ErrorType::Picture_Hex_Too_Little,
                          ErrorSystem::ErrorLevel::Recoverable,
                          ErrorSystem::ErrorCategory::Picture,
                          (ws + L"圖片區塊中 hex 內容過短!"));
        tasks.push_back({start,end,"[[IMG_ERR_" + std::to_string(pictCount_) + "]]"});
        std::string logstr = std::string("第 ") + std::to_string(pictCount_) + 
                             std::string(" 圖片區塊 hex 內容過短,跳過處理");
        logger.log(LogLevel::Warn,logstr);
        pictCount_++;
        pos = rtfContent.find("{\\pict",end);// 找到下一個的開頭
        continue;
      }
      
      
      
      std::string_view header(groupView.data(),hexstart);
      
      PictHeaderInfo headerinfo = parsePictHeader(header);
      if(headerinfo.format == PictFormat::UNKNOWN){
        std::wstring ws = L"第" + std::to_wstring(pictCount_);
        PictErrorInfo.add(ErrorSystem::ErrorType::Picture_Content_FormatError,
                          ErrorSystem::ErrorLevel::Recoverable,
                          ErrorSystem::ErrorCategory::Picture,
                          (ws + L"圖片區塊中無法正確判斷圖片格式!"));
        tasks.push_back({start,end,"[[IMG_ERR_" + std::to_string(pictCount_) + "]]"});
        std::string logstr = std::string("第 ") + std::to_string(pictCount_) + 
                             std::string(" 圖片區塊無法正確判斷圖片格式,跳過處理");
        logger.log(LogLevel::Warn,logstr);
        pictCount_++;
        // 找到下一個的開頭
        pos = rtfContent.find("{\\pict",end);
        continue;
      }
      
      PictInfo info;
      info.format = headerinfo.format;
      std::string_view hexView(groupView.data()+hexstart,end - hexstart);
      info.bytes  = extractHexBytes(hexView);
      if(info.bytes.empty()){
        std::wstring ws = L"第" + std::to_wstring(pictCount_);
        PictErrorInfo.add(ErrorSystem::ErrorType::Picture_Content_DataError,
                          ErrorSystem::ErrorLevel::Recoverable,
                          ErrorSystem::ErrorCategory::Picture,
                          (ws + L"圖片區塊 hex 內容解析失敗 (無有效位元組)!"));
        tasks.push_back({start,end,"[[IMG_ERR_" + std::to_string(pictCount_) + "]]"});
        std::string logstr = std::string("第 ") + std::to_string(pictCount_) + 
                             std::string(" 圖片區塊 hex 內容解析失敗 (無有效位元組)!,跳過處理");
        logger.log(LogLevel::Warn,logstr);
        pictCount_++;
        // 找到下一個的開頭
        pos = rtfContent.find("{\\pict",end);
        continue;
      }
      
      auto outPathOpt = makeOutputPath(info.format,pictCount_);
      if(!outPathOpt){
        std::wstring ws = L"第" + std::to_wstring(pictCount_);
        PictErrorInfo.add(ErrorSystem::ErrorType::Picture_Output_Path_Failed,
                          ErrorSystem::ErrorLevel::Recoverable,
                          ErrorSystem::ErrorCategory::Picture,
                          (ws + L"圖片輸出路徑建立失敗!"));
        tasks.push_back({start,end,"[[IMG_ERR_" + std::to_string(pictCount_) + "]]"});
        std::string logstr = std::string("第 ") + std::to_string(pictCount_) + 
                             std::string(" 圖片輸出路徑建立失敗!,跳過處理");
        logger.log(LogLevel::Warn,logstr);
        pictCount_++;
        // 找到下一個的開頭
        pos = rtfContent.find("{\\pict",end);
        continue;
      }
      info.outputPath = *outPathOpt;
      if(!imgProcessor_.process(info,true)){
        std::wstring ws = L"第" + std::to_wstring(pictCount_);
        PictErrorInfo.add(ErrorSystem::ErrorType::Picture_File_Creation_Failed,
                          ErrorSystem::ErrorLevel::Recoverable,
                          ErrorSystem::ErrorCategory::Picture,
                          (ws + L"圖片建立成圖檔失敗!"));
        tasks.push_back({start,end,"[[IMG_ERR_" + std::to_string(pictCount_) + "]]"});
        std::string logstr = std::string("第 ") + std::to_string(pictCount_) + 
                             std::string(" 圖片建立成圖檔失敗!,跳過處理");
        logger.log(LogLevel::Warn,logstr);
        pictCount_++;
        // 找到下一個的開頭
        pos = rtfContent.find("{\\pict",end);
        continue;
      }
      tasks.push_back({start,end,"{\\*\\imgblock[[IMG_"+std::to_string(pictCount_)+"]]}"});
      pictCount_++;
      
      // 找到下一個的開頭
      pos = rtfContent.find("{\\pict",end); 
    }
   
    
    logger.log(LogLevel::Info,"圖片區塊轉換為圖檔完成,開始進行區塊替代為標記符作業");
    // 從後往前替換，避免索引位移 
    if(!tasks.empty()){
      for(int i = tasks.size();i > 0;i--){
        ReplaceTask target = tasks[i-1];
        rtfContent.replace(target.start,target.end-target.start,target.text);
      }
    }
    
    {
      std::string logstr = std::string("圖片區塊替代為標記符作業完成,總共處理了: ") + 
                           std::to_string(pictCount_) + 
                           std::string(" 張圖片"); 
      logger.log(LogLevel::Info,logstr);
    }

    // 如果有錯誤資訊紀錄進 errorlog 中 
    if(!PictErrorInfo.isEmpty()){
      errorHandler_.handle(PictErrorInfo);
      return PictProcessResult::SkipPict;
    }
    
    return PictProcessResult::OK;
}
GroupScanReport PictDisassembler::scanGroup(const std::string& text,size_t groupStart){
    GroupScanReport report;
    size_t braceCount = 1;
    size_t pos = groupStart + 1;
    bool inTheHex = false;
    size_t hexCount = 0;
    auto isHexDigit = [](char c){
     return (c >= '0' &&  c <= '9') ||
            (c >= 'a' &&  c <= 'f') ||
            (c >= 'A' &&  c <= 'F'); 
    };

    auto isHexWhitespace = [](char c){
      return c == ' ' || c == '\t' || c == '\r' || c == '\n';
    };

    auto isParControl = [&](size_t p){
      return p + 3 < text.size() &&
             text[p]   == '\\'&&
             text[p+1] == 'p' &&
             text[p+2] == 'a' &&
             text[p+3] == 'r' &&
             (p+4 >= text.size() ||
             !std::isalpha(static_cast<unsigned char>(text[p+4])));
    };

    while(pos < text.size()){
      
      // 在判定 hex 區塊損壞後 在次遇到群組開頭或 \\par 就停止迴圈 進入大括號平衡判定
      if(report.HexBroken){
        if(text[pos] == '{' && pos + 1 < text.size() && text[pos +1] == '\\'){ // 進入損壞狀態且遇到新的群組開頭
          report.braceBalanced = false; // pict 無法再延伸
          return report; 
        }

        if(isParControl(pos))
        {
          report.braceBalanced = false; // pict 無法再延伸
          return report; 
        }
      }
      
      if(inTheHex && text[pos] == '\\'){ // 在進入連續 hex 區域後, 碰到控制符就更新狀態 
        report.pictHexInterrupted = true;
        if(isParControl(pos)){
          report.HexBroken = true;
        }
      }

      if(inTheHex && text[pos] == '{'){ // hex 區域不允許巢狀群組,遇到就判定為超出邊界
        report.detectedNewGroupStart = true;
        report.HexBroken = true;
      }
      
      if(text[pos] == '{'){
        braceCount++;
      }else if(text[pos] == '}'){
        braceCount--;
        if(braceCount == 0){
          pos++; // 確保會在大括號後面一位
          break; // 以達到平衡終止迴圈,迴圈外會進行回傳工作
        }
      }

      if(!report.HexBroken){
        if(isHexDigit(text[pos])){
          hexCount++;
        }else if(isHexWhitespace(text[pos])){
        // 遇到空格或換行 不影響連續 hex 判斷
        }else{
          hexCount = 0;
          inTheHex = false;
        }

        if(hexCount >= 32){
          inTheHex = true;
        }
      }
      

      pos++;
    }

    if(braceCount != 0){
      report.braceBalanced = false;
      return report;
    }
    
    report.groupEnd = pos;
    return report;
}
size_t PictDisassembler::findPictHexStart(std::string_view text,size_t start) const{
    size_t end = start + 1;
    
    while(end < text.size()){
      bool isrealend = false;
      
      if(text[end] == '\\'){
        end++;
        while(end < text.size() && std::isalpha(static_cast<unsigned char>(text[end]))) end++;
        if(end < text.size() && text[end] == '-') end++;
        while(end < text.size() && std::isdigit(static_cast<unsigned char>(text[end]))) end++;
        while(end < text.size() && std::isspace(static_cast<unsigned char>(text[end]))) end++;
        continue;
      } 
      if(text[end] == '*' || text[end] == '{' || text[end] == '}'){
        end++;
        continue;
      }
      if (std::isspace(static_cast<unsigned char>(text[end]))) {
        ++end;
        continue;
      }
      if(std::isxdigit(static_cast<unsigned char>(text[end]))){
        isrealend = true;
        if(end + 7 < text.size()){
          for(size_t i = 1 ; i <= 7;i++){
            if(!std::isxdigit(static_cast<unsigned char>(text[end + i]))){
              isrealend = false;
              break;
            }
          }
        }else{
          isrealend = false;
        }
      }

      if(isrealend) return end;
      
      end++;
    }

    return std::string::npos; // 全部跑完(高機率為錯誤)
}
bool PictDisassembler::isWmfHexTooShort(std::string_view hex){
    constexpr size_t MIN_WMF_HEX_DIGITS = 96;
    size_t hexDigits = 0;
    for (unsigned char c : hex) {
      if (std::isxdigit(c)) {
        ++hexDigits;
        if (hexDigits >= MIN_WMF_HEX_DIGITS) {
          return false; // 已經夠長，不用再數
        }
      }
    }

    return true;
}
PictHeaderInfo PictDisassembler::parsePictHeader(std::string_view header){
    PictHeaderInfo info;
    info.format   = detectPictFormat(header);
    info.picw     = readControlInt(header, "\\picw");
    info.pich     = readControlInt(header, "\\pich");
    info.picwgoal = readControlInt(header, "\\picwgoal");
    info.pichgoal = readControlInt(header, "\\pichgoal");
    return info;
}
PictFormat PictDisassembler::detectPictFormat(std::string_view header){
    if (header.find("\\pngblip") != std::string_view::npos)
        return PictFormat::PNG;
    if (header.find("\\jpegblip") != std::string_view::npos)
        return PictFormat::JPEG;
    if (header.find("\\emfblip") != std::string_view::npos)
        return PictFormat::EMF;
    if (header.find("\\wmetafile") != std::string_view::npos)
        return PictFormat::WMF;
    return PictFormat::UNKNOWN;
}
std::optional<int> PictDisassembler::readControlInt(std::string_view header, std::string_view key){
    size_t pos = header.find(key);
    if (pos == std::string_view::npos) return std::nullopt;

    pos += key.size(); // 跳過 key 本身
    if (pos >= header.size()) return std::nullopt;

    // 可選負號
    bool neg = false;
    if (header[pos] == '-') { 
      neg = true; ++pos; 
    }

    if (pos >= header.size() || !std::isdigit((unsigned char)header[pos]))
        return std::nullopt;

    int val = 0;
    while (pos < header.size() && std::isdigit((unsigned char)header[pos])) {
        val = val * 10 + (header[pos] - '0');
        ++pos;
    }
    return neg ? -val : val;
}
int PictDisassembler::hexValue(unsigned char c){
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return -1;
}
std::vector<uint8_t> PictDisassembler::extractHexBytes(std::string_view hexView){
    std::vector<uint8_t> bytes;
    bytes.reserve(hexView.size() / 2); // 預設省資源

    int highNibble = -1;

    for(size_t i = 0; i < hexView.size();i++){
      unsigned char c = static_cast<unsigned char>(hexView[i]);

      if(c == '}') break; // 遇到結尾

      if(std::isspace(c)) continue;
    
      int hv = hexValue(c);// 解析 hex digit
      if(hv < 0){
        continue; // 遇到非 hex 字元 這裡採取跳過策略
      }

      if(highNibble < 0){
        highNibble = hv;
      }else{
        uint8_t b = static_cast<uint8_t>((highNibble << 4) | hv);
        bytes.push_back(b);
        highNibble = -1;
      }
    }
    
    return bytes;
}
std::optional<std::filesystem::path> PictDisassembler::makeOutputPath(PictFormat fmt,size_t index){
    std::string ext = ".bin";
    switch(fmt){
      case PictFormat::PNG:  ext = ".png";  break;
      case PictFormat::JPEG: ext = ".jpg";  break;
      case PictFormat::WMF:  ext = ".wmf";  break;
      case PictFormat::EMF:  ext = ".emf";  break;
      default: ext = ".img"; break;
    }

    if(!outputDir_.ensure()){
      return std::nullopt;
    }

    return outputDir_.path() / ("IMG_" + std::to_string(index) + ext);
}