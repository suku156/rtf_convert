// =====================================================
// 多檔案會用到的 enum
// Used by :
//   CliParser
//   ErrorDetector
//   RtfFormatDetection
//   RTFProcessor
//   MyThread
//   OutputDirGuard
//  
// =====================================================

#pragma once
enum class Encoding{
  UTF8_BOM,
  UTF8_NoBOM,
  UTF16_LE,
  UTF16_BE,
  UTF32_LE,
  UTF32_BE,
  Ansi_Big5,
  Ansi_GBK,
  Ansi_JIS,
  Ansi_Korean,
  Ansi_Latin_1,
  Ansi_CEI,
  Ansi_Cyrillic,
  Invalid
};

enum class DetectorResult{
  isOK,               
  NotRTF,             
  HeaderMalformed,    
  UnsupportedEncoding,
  EncodingConflict,   
  CannotDetect,       
  EmptyFile,         
  FileTooSmall,       
  BinaryFile,         
  WrongStep,          
  Invalid             
};

enum class DetectorCategory{
  OK,           
  Warning,     
  Recoverable,  
  Fatal,        
  InternalError 
};


namespace Common{
  // Cli 系統 跟 主流程(RTFProcessor) 都會用到
  enum class OutputFormat{
    Txt,
    Md,
    Html
  };
  
  // 用於表示對同名資料夾的預設處理
  enum class ExistingDirPolicy {
    // 有同名檔案就拒絕
    Reject,   
    // 有同名檔案就把舊的覆蓋掉
    Overwrite,
    // 在尾部加上數字並建立新檔案 
    RenameWithSuffix
  }; 
}