// =====================================================
// Module : RTFProcessor (implementation)
// =====================================================
#include "RTFProcessor.h"
#include "Universal_Module/OutputDirGuard.h" // 用來確保輸出資料夾的模組
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
#include "MainProcess_Module/FileProcessRequest.h"
#include "Feedback_Module/ProgressEvent.h"
#include "Feedback_Module/IProgressObserver.h"
#include "RtfTableProcessor_Module/RtfTablePreProcessor.h"
#include "RtfGroupProcessor_Module/RtfPreProcessor.h"
#include<filesystem>
#include<optional>
#include<iostream>
#ifdef _WIN32
  #include <windows.h>
#endif

// 使用 namespace 的隱常功能
// 目前只要主流程會用到的類別與函式,先放在.cpp 中 有需求時在拆出去
namespace{
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

}

// 主流程函式定義
ProcessResult RTFProcessor::processFile(const FileProcessRequest& req)
{
    const std::filesystem::path& filePath = req.filePath;
    auto outputformat = req.outputFormat;

    ProcessResult processresult = ProcessResult::SuccessClean;

    // 簡單測試最終路徑
    if(req.finalOutputDir.empty()){
      notify(ProgressEvent{
        ProgressEventType::Error,
        L"輸出資料夾路徑為空"
      });
      processresult = ProcessResult::Failed;
      return processresult;
    }
    if(req.finalOutputPath.empty()){
      notify(ProgressEvent{
        ProgressEventType::Error,
        L"輸出檔案路徑為空"
      });
      processresult = ProcessResult::Failed;
      return processresult;
    }
    if(req.finalOutputPath.parent_path() != req.finalOutputDir){
      notify(ProgressEvent{
        ProgressEventType::Error,
        L"輸出檔案路徑與輸出資料夾路徑不一致"
      });
      processresult = ProcessResult::Failed;
      return processresult;
    }
    
    // 建立輸出資料夾
    OutputDirGuard fileOut(req.finalOutputDir,req.dirPolicy);
    auto dirResult = fileOut.ensure();
    if(dirResult != EnsureDirResult::Success){
      switch(dirResult){
        case EnsureDirResult::AlreadyExists:
        notify(ProgressEvent{
          ProgressEventType::Error,
          L"輸出資料夾已存在: " + req.finalOutputDir.wstring()
        });
        break;
        case EnsureDirResult::NotDirectory:
        notify(ProgressEvent{
          ProgressEventType::Error,
          L"輸出路徑已存在但不是資料夾: " + req.finalOutputDir.wstring()
        });
        break;
        case EnsureDirResult::CreateFailed:
        notify(ProgressEvent{
          ProgressEventType::Error,
          L"建立輸出資料夾失敗: " + req.finalOutputDir.wstring()
        });
        break;
        case EnsureDirResult::VerifyFailed:
        notify(ProgressEvent{
          ProgressEventType::Error,
          L"建立後驗證輸出資料夾失敗: " + req.finalOutputDir.wstring() 
        });
        break;
        default:
        notify(ProgressEvent{
          ProgressEventType::Error,
          L"未知的輸出資料夾錯誤: " + req.finalOutputDir.wstring()
        });
        break;
      }
      processresult = ProcessResult::Failed;
      return processresult;
    }
    notify(ProgressEvent{
      ProgressEventType::Info,
      L"輸出資料夾建立成功"
    });
    logSystem logger;
    
    if(!logger.open(fileOut.path())){
      notify(ProgressEvent{
        ProgressEventType::Error,
        L"log 系統無法開啟"
      });
      processresult = ProcessResult::Failed;
      return processresult;
    }
    logger.log(LogLevel::Info,"以確保有輸出資料夾: " + req.finalOutputDir.string());

    notify(ProgressEvent{
      ProgressEventType::Info,
      L"開始處理目標檔案: " + filePath.wstring()
    });
    logger.log(LogLevel::Info,std::string("正在處理檔案: ") + pathToUtf8(filePath));

    ErrorHandle errorhandler(logger);
    errorhandler.beginFile();
    FileScopeGuard guard{errorhandler}; // 使用類別的解構功能自動呼叫endfile()
    logger.log(LogLevel::Info,"錯誤系統啟用 :(ErrorHandle 模組)");
    
    
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
      processresult = ProcessResult::Failed;
      return processresult;
     }
     logger.log(LogLevel::Info,"成功開啟輸入檔案: " + pathToUtf8(filePath.string()));
     
    // 將輸入檔案讀成 string 好做之後的加工
    std::string rtfContent{
      (std::istreambuf_iterator<char>(input)),
       std::istreambuf_iterator<char>()
    };

    //偵測檔案,並回傳偵測結果結構
    logger.log(LogLevel::Info,"輸入檔案偵測開始");
    EncodingDetector detect;
    DetectOutputNew detectoutput = detect.detectEncoding(input);
    
    if(detectoutput.sizeInfo.isTooShort){
      std::string str = "檔案過短, 預設最小檔案長度: " + 
                        std::to_string(detectoutput.sizeInfo.minRequiredSize) +
                        " , 實際偵測到檔案長度: " +
                        std::to_string(detectoutput.sizeInfo.fileSize);
      logger.log(LogLevel::Error,str);
      notify(ProgressEvent{
        ProgressEventType::Error,
        L"檔案長度低於預設值"
      });
      processresult = ProcessResult::Failed;
      return processresult;
    }
    
    if(!detectoutput.hasValidRtfRootHeader){
      logger.log(LogLevel::Error,"檔案關鍵標記符 {\\rtfX 缺失或不符合規定");
      notify(ProgressEvent{
        ProgressEventType::Error,
        L"檔案偵測時關鍵標記符錯誤"
      });
      processresult = ProcessResult::Failed;
      return processresult;
    }

    // 圖片區況處理
    PictDisassembler pictdisassemcler(errorhandler,fileOut);
    auto picrResult = pictdisassemcler.process(rtfContent,logger);
    switch(picrResult){
      case PictProcessResult::OK :
      logger.log(LogLevel::Info,"圖片區塊正確結束");
      notify(ProgressEvent{
        ProgressEventType::Info,
        L"圖片處理區塊完成"
      });
      break;
      case PictProcessResult::SkipPict :
      logger.log(LogLevel::Warn,"圖片區塊有部份進行錯誤區塊跳過處理,但是可繼續後面流程");
      notify(ProgressEvent{
        ProgressEventType::Warning,
        L"圖片處理區塊有部份區塊錯誤進行跳過處裡"
      });
      if(processresult != ProcessResult::Failed){
        processresult = ProcessResult::SuccessWithWarning;
      }
      break;
      case PictProcessResult::AbortFile:
      logger.log(LogLevel::Error,"圖片區塊有無法跳過處裡之錯誤!,終止後續流程");
      notify(ProgressEvent{
        ProgressEventType::Error,
        L"圖片處理區塊錯誤流程中止"
      });
      processresult = ProcessResult::Failed;
      return processresult;
      case PictProcessResult::AbortGlobal:
      logger.log(LogLevel::Error,"圖片區塊有破壞性之錯誤!,終止整個程序!");
      notify(ProgressEvent{
        ProgressEventType::Error,
        L"圖片處理區塊錯誤整體程序中止"
      });
      errorhandler.handleFatalGlobal(L"圖片區塊破壞性錯誤導致整體程式緊急終止!!!");
    }
  
    //表格區塊需要的預先處裡
    logger.log(LogLevel::Info,"處裡表格區塊預先處理的部份");
    sheetProcessor sheetprocessor;
    sheetprocessor.processor(rtfContent);

    // 進行解讀前的純控制群組處理與篩選
    RtfPreProcessor rtfPreProcessor(logger);
    rtfContent = rtfPreProcessor.process(rtfContent);

    notify(ProgressEvent{
      ProgressEventType::Info,
      L"開始嘗試解碼訊息"
    });

    
    // 先處理可能存在的 \uXXXX 區域部份
    
    logger.log(LogLevel::Info,"處裡 utf \\uXXXX 標記符");
    auto utf_decoded = utf8Decoder().decode(rtfContent);
    if(!utf_decoded){
      processresult = ProcessResult::Failed;
      return processresult;
    } 
    rtfContent = utf_decoded.value();
    

    // 接著處裡 \'XX 區域部份 
    logger.log(LogLevel::Info,"嘗試處裡 ansi \\'XX 標記符");
    Encoding enc = Encoding::Invalid;
    if(detectoutput.ansiCodePage){
      switch(*detectoutput.ansiCodePage){
        case 950:
        enc = Encoding::Ansi_Big5;
        break;
        case 936:
        enc = Encoding::Ansi_GBK;
        break;
        case 932:
        enc = Encoding::Ansi_JIS;
        break;
        case 949:
        enc = Encoding::Ansi_Korean;
        break;
        case 1252:
        enc = Encoding::Ansi_Latin_1;
        break;
        case 1250:
        enc = Encoding::Ansi_CEI;
        break;
        case 1251:
        enc = Encoding::Ansi_Cyrillic;
        break;
      }

      AnsiErrorDetector detector(enc);
      ansiDecoder ansiDe(observer_);
      auto ansi_decoded = ansiDe.setCodePage(*detectoutput.ansiCodePage).decode(rtfContent,detector,errorhandler); // 使用串聯特性
      if(!ansi_decoded.str){
        processresult = ProcessResult::Failed;
        return processresult;
      } // 偵測結果錯誤就中斷函式執行
      rtfContent = *ansi_decoded.str;
      if(ansi_decoded.state == decodeState::SuccessWithWarning){
        processresult = ProcessResult::SuccessWithWarning;
      }
    }else{
      logger.log(LogLevel::Info, "未偵測到 ANSI codepage,跳過 \\'XX 解碼");
    }

    // 處裡群組前進行一次 utf8 檢查
    Utf8ErrorDetector utf_detector;
    ErrorSystem::ErrorInfo info = utf_detector.detectText(rtfContent);
    if(!info.isEmpty()){
      errorhandler.handle(info);
      notify(ProgressEvent{
        ProgressEventType::Warning,
        L"處裡完的字串出現部份警告,詳情請查看輸出資料夾內的 log"
      });
      if(processresult != ProcessResult::Failed){
        processresult = ProcessResult::SuccessWithWarning;
      }
      if(info.getMaxLevel() > ErrorSystem::ErrorLevel::Recoverable){
        notify(ProgressEvent{
          ProgressEventType::Error,
          L"處裡完的字串出現嚴重錯誤中止流程"
        });
        processresult = ProcessResult::Failed;
        return processresult;
      }
    }

    // 處裡群組
    utf8GroupProcessor().processGroups(rtfContent);
    
    
    logger.log(LogLevel::Info,"清理轉換後之字串剩餘之控制符");
    //用類別的功能來清除文字中剩餘的控制碼等等
    textRtfProcessor textrtfprocessor(observer_);
    textrtfprocessor.Processor(rtfContent,logger);
    notify(ProgressEvent{
      ProgressEventType::Info,
      L"完成多餘控制符清理"
    });

    
    logger.log(LogLevel::Info,"開始檢查檔案處裡結果品質");
    //檢查 圖檔區域處裡完的純文字的品質檢查
    auto GeneralInfo = GeneralErrorDetector().detect(rtfContent);
    if(!GeneralInfo.isEmpty()){
      notify(ProgressEvent{
        ProgressEventType::Warning,
        L"檔案已完成轉換，但最終輸出可能仍包含未完全清理的 RTF 內容"
      });
      errorhandler.handle(GeneralInfo);
      if(processresult != ProcessResult::Failed){
        processresult = ProcessResult::SuccessWithWarning;
      }
    }
    
    notify(ProgressEvent{
      ProgressEventType::Info,
      L"處裡結果品質檢查結束"
    });
    
    
    logger.log(LogLevel::Info,"嘗試開啟輸出檔案");

    // 直接使用得到的最終檔案路徑
    std::ofstream output(req.finalOutputPath, std::ios::binary);
    if (!output) {
      ErrorSystem::ErrorInfo info;
      info.add(ErrorSystem::ErrorType::Fatal_CreatOutputFail,
               ErrorSystem::ErrorLevel::FatalLocal,
               ErrorSystem::ErrorCategory::FileIO,
               L"無法建立輸出檔案 : " + req.finalOutputPath.wstring());
      errorhandler.handle(info);
      processresult = ProcessResult::Failed;
      return processresult;
    }
    logger.log(LogLevel::Info,"輸出檔案開啟成功");
    notify(ProgressEvent{
      ProgressEventType::Info,
      L"成功建立輸出檔案"
    });

    // .txt 檔案寫入 UTF-8 BOM 讓 Windows 筆記本正常顯示
    if(outputformat == Common::OutputFormat::Txt){
      const unsigned char bom[] = {0xEF, 0xBB, 0xBF};
      output.write(reinterpret_cast<const char*>(bom), 3);
    }
 
    //output << rtfContent;
    
    
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
      notify(ProgressEvent{
        ProgressEventType::Error,
        L"[Unknown OutputFormat]"
      });
      processresult = ProcessResult::Failed;
      return processresult;
    }
    
    
    notify(ProgressEvent{
      ProgressEventType::Info,
      L"已輸出至: " + req.finalOutputPath.parent_path().wstring()
    });
    output.close();
    logger.log(LogLevel::Info,"關閉輸出檔案");
    
    logger.close();
    return processresult;
}

void RTFProcessor::notify(const ProgressEvent& event){
  if(observer_){
    observer_->onEvent(event);
  }
}
