// =====================================================
// Module : ErrorSystem (implementation)
// enum 印出什麼資訊存放在此
// =====================================================
#include "ErrorSystem.h"

namespace ErrorSystem{
  
  //ErrorLevel 錯誤等級定義
  const std::array<const char*,5> ErrorLevelNames = {
    "None",
    "Warning",
    "Recoverable",
    "FatalLocal",
    "FatalGlobal"
  };
  std::ostream& operator<< (std::ostream& os , ErrorLevel level){
    auto index = static_cast<size_t>(level);
    if(index < ErrorLevelNames.size()){
      return os << ErrorLevelNames[index]; 
    }
    return os << "Unknown";
  }

  // ErrorCategory 分類錯誤區塊定義
  const  std::array<const char*,8> ErrorCategoryName = {
     "None",
     "FileIO",
     "DetectEncoding",
     "Decode",
     "Picture",
     "Group",
     "TextClean",
     "detector" 
   };
  std::ostream& operator<< (std::ostream& os , ErrorCategory category){
    auto index = static_cast<size_t>(category);
    if(index <  ErrorCategoryName.size()){
      return os <<  ErrorCategoryName[index]; 
    }
    return os << "Unknown";
  }

  // ErrorType 錯誤型態定義
  const std::array<const char*,34> ErrorTypeName = {
    "OK",
    "Type_Convert_Fail",
    "Fatal_NotRTF",
    "Fatal_EncodingUnreadable",
    "Fatal_CraeateDirFail",
    "Fatal_FileOpenFail",
    "Fatal_CreatOutputFail",
    "Decode_UTF8_BrokenSequence",
    "Decode_UTF8_ByteError",
    "Decode_ANSI_InvalidByte",
    "Decode_ANSI_TruncatedLeadByte",
    "Decode_ANSI_codeLacking",
    "Decode_UTF16_ByteError",
    "Picture_Structural_BraceMismatch",
    "Picture_Content_InvalidFormat",
    "Picture_Content_TooLarge",
    "Picture_Content_DataError",
    "Picture_Content_FormatError",
    "Picture_StructureError",
    "Picture_Retrieve_Data_Failed",
    "Picture_Output_Path_Failed",
    "Picture_File_Creation_Failed",
    "Picture_Hex_Too_Little",
    "Picture_Hex_Broken",
    "Group_BraceMismatch",
    "Group_UnknownControlWord",
    "TextClean_InvalidUnicode",
    "TextClean_UnexpectedControl",
    "Detector_Warning",
    "Detector_Recoverable",
    "Detector_Fatal",
    "General_Fatal",
    "General_Recoverable",
    "General_Warning"
  };
  std::ostream& operator<< (std::ostream& os , ErrorType type){
    auto index = static_cast<size_t>(type);
    if(index <  ErrorTypeName.size()){
      return os <<  ErrorTypeName[index]; 
    }
    return os << "Unknown";
  }

  // ErrorItem 存放集合宣告
  std::ostream& operator<<(std::ostream& os,const ErrorItem& item){
    os << "[Category]: " << item.category << "\n"
       << "[Type]: "     << item.type     << "\n"
       << "[Level]: "    << item.level    << "\n";
    
    std::string utf8 = toUtf8(item.message);
    os << "[message]: "  << utf8          << "\n";
    
    return os;
  }

  // ErrorInfo 主要使用目標定義
  bool ErrorInfo::isEmpty() const{
    return items_.empty();
  }
  size_t ErrorInfo::getErrorCount() const{
    return items_.size();
  }
  size_t ErrorInfo::getWarringCount() const{
    size_t Count = 0;
    if(items_.empty()) return 0;
    for(auto p : items_){
      if(p.level == ErrorSystem::ErrorLevel::Warning){
        Count++;
      }
    }
    return Count;
  }
  // 可以直接用用一個個值加入
  void ErrorInfo::add(ErrorSystem::ErrorType type,ErrorSystem::ErrorLevel level,
                      ErrorSystem::ErrorCategory category, std::wstring message)
  {
    items_.push_back({type,level,category,std::move(message)});
  }
  // 也可以直接一個整體放入
  void ErrorInfo::add(const ErrorSystem::ErrorItem& item){
    items_.push_back(item);
  }
  // 取得所有錯誤
  const std::vector<ErrorSystem::ErrorItem>& ErrorInfo::getItems() const{
    return items_;
  }
  // 用來找到最嚴重的錯誤等級
  ErrorLevel ErrorInfo::getMaxLevel() const{
    if(items_.empty()) return ErrorLevel::None;

    ErrorLevel result = ErrorLevel::None;

    for(const auto& item : items_){
      if(static_cast<int>(item.level) > static_cast<int>(result)){
        result = item.level;
      }
    }

    return result;
  }
  // 用來合併另一個 ErrorInfo
  void ErrorInfo::merge(const ErrorInfo& other) {
    items_.insert(items_.end(), other.items_.begin(), other.items_.end());
  }

  std::ostream& operator<<(std::ostream& os,const ErrorInfo& info){
    for(const auto& p : info.getItems()){
      os << p << "\n";
    }

    return os;
  }
}