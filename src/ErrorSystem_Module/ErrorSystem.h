// =====================================================
// Module  : ErrorSystem
// Author  : suku156
// Purpose : 用來存放錯誤系統會用的資訊
// Layer   : errorsystem
//
// Depend  :
//    
// Used by :
//   RTFProcessor   
//   Decode
//   ErrorDetector
//   ErrorHandle
//   pictprocessor
//   logsystem
//
// Notes :
//   錯誤系統中用來存放所有會用的資訊(層級,地點,解釋等等)
//   
// =====================================================

#pragma once
#include<array>
#include<string_view>
#include<ostream>
#include<locale>
#include<codecvt>
#include<vector>


namespace ErrorSystem{

  //ErrorLevel 分類錯誤等級宣告
  enum class ErrorLevel{
    None,       // 無錯
    Warning,    // 警告(不影響執行)
    Recoverable,// 影響某個區塊,但可以繼續執行流程
    FatalLocal, // 區域型致命錯誤,需中斷當下目標之執行(非整個程式)
    FatalGlobal // 全域型致命錯誤,需直接中止整個程式
  };
  extern const std::array<const char*,5> ErrorLevelNames;
  std::ostream& operator<<(std::ostream& os,ErrorLevel level);
  constexpr std::string_view getLevelStr(ErrorLevel level) noexcept{
    auto index = static_cast<size_t>(level);
    if(index < ErrorLevelNames.size()){
      return ErrorLevelNames[index];
    }

    return "UnKnowErrorLevel";
  }
  
  // ErrorCategory 分類錯誤區塊宣告
  enum class ErrorCategory{
    None,
    FileIO,
    DetectEncoding,
    Decode,
    Picture,
    Group,
    TextClean
  };
  extern const  std::array<const char*,7> ErrorCategoryName;
  std::ostream& operator<<(std::ostream& os , ErrorCategory category);
  constexpr std::string_view getCategoryStr(ErrorCategory category) noexcept{
    auto index = static_cast<size_t>(category);
    if(index <  ErrorCategoryName.size()){
      return ErrorCategoryName[index]; 
    }
    return "UnknownErrorCategory";
  }

  // ErrorType 錯誤型態宣告
  enum class ErrorType{
    OK,
    Type_Convert_Fail,
    // ========= Fatal Errors =========
    NotRTF,
    EncodingUnreadable,
    Fatal_CreateDirFail,
    Fatal_FileOpenFail,
    Fatal_CreatOutputFail,

    // ========= Decode Errors (Recoverable) =========
    Decode_UTF8_BrokenSequence,
    Decode_UTF8_ByteError,
    Decode_ANSI_InvalidByte,
    Decode_ANSI_TruncatedLeadByte,
    Decode_ANSI_codeLacking,
    Decode_UTF16_ByteError,

    // ========= Picture Errors (Recoverable) =========
    Picture_Structural_BraceMismatch,
    Picture_Content_InvalidFormat,
    Picture_Content_TooLarge,
    Picture_Content_DataError,
    Picture_Content_FormatError,
    Picture_StructureError,
    Picture_Retrieve_Data_Failed,
    Picture_Output_Path_Failed,
    Picture_File_Creation_Failed,
    Picture_Hex_Too_Little,
    Picture_Hex_Broken,

    // ========= Group Errors =========
    Group_BraceMismatch,
    Group_UnknownControlWord,

    // ========= TextClean Errors =========
    TextClean_InvalidUnicode,
    TextClean_UnexpectedControl,

    // ========= Detector Errors =====
    Detector_Warning,
    Detector_Recoverable,
    Detector_Fatal,

    // ========= RTF General ======
    General_Fatal,
    General_Recoverable,
    General_Warning
  };
  extern const std::array<const char*,34> ErrorTypeName;
  std::ostream& operator<<(std::ostream& os , ErrorType type);
  constexpr std::string_view getTypeStr(ErrorType type) noexcept{
    auto index = static_cast<size_t>(type);
    if(index <  ErrorTypeName.size()){
      return ErrorTypeName[index]; 
    }
    return "UnknownErrorType";
  }

  // ErrorItem 存放集合宣告
  struct ErrorItem{
    ErrorSystem::ErrorType type;
    ErrorSystem::ErrorLevel level;
    ErrorSystem::ErrorCategory category;
    std::wstring message;
  };
  inline std::string toUtf8(const std::wstring& ws){
    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
    return conv.to_bytes(ws);
  }
  std::ostream& operator<<(std::ostream& os,const ErrorItem& item);

  // ErrorInfo 主要使用目標宣告
  class ErrorInfo{
    std::vector<ErrorSystem::ErrorItem> items_;
  public:
    bool isEmpty() const;
    size_t getErrorCount() const;
    size_t getWarringCount() const;
    void add(ErrorSystem::ErrorType type,ErrorSystem::ErrorLevel level,
             ErrorSystem::ErrorCategory category, std::wstring message);
    void add(const ErrorSystem::ErrorItem& item);
    const std::vector<ErrorSystem::ErrorItem>& getItems() const;
    ErrorLevel getMaxLevel() const;
    void merge(const ErrorInfo& other);
  };

  std::ostream& operator<<(std::ostream& os,const ErrorInfo& info);

}