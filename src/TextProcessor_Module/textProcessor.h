// =====================================================
// Module  : textProcessor
// Author  : suku156
// Purpose : 清理多餘控制符
// Layer   : textProcessor
//
// Depend  :
//   LogSystem
//   Console
//   
// Used by :
//   RTFProcessor   
//
// Notes :
//   將解碼完的字串的控制符清理乾淨
//   本清理需要再解碼完才使用避免誤刪需要之控制符
// =====================================================
#pragma once
#include<string>
#include<string_view>
#include<cstddef>


// forowrd delecration
class logSystem;
class IProgressObserver;
struct ProgressEvent;

class textRtfProcessor{
  enum class GroupDecision{
    Drop,
    KeepText,
    KeepAll,
    Field,
    Start
  };
  IProgressObserver* observer_ = nullptr;
public:
  explicit textRtfProcessor(IProgressObserver* observer = nullptr) : observer_(observer) {}
  void Processor(std::string& Cleaned,logSystem& logger);
private:
  inline bool checkAsciiScope(const std::string& text, size_t pos ,size_t len,bool tailOnly = false);
  inline bool isControlCharacter(char c);
  bool start_with(std::string_view s,const char* prefix);
  bool isKnownDroppableGroup(std::string_view g);
  inline bool isHex(char c);
  bool hasVisibleText(std::string_view g);
  GroupDecision classifyGroup(std::string_view group);
  void controlGroupProcessor(std::string& Cleaned,logSystem& logger);
  void finalRtfSymbolClean(std::string& Cleaned);
  void compressBlankLines(std::string& Cleaned,int maxBlankLines);
  void removeOuterBraces(std::string& Cleaned);
  void trimLeadingNewlines(std::string& Cleaned);
  void removeProtectionSymbol(std::string& Cleaned);
  bool isSkippableRtfEscape(const std::string& s, size_t pos);
  std::string replaceSemanticControls(std::string_view target);
  std::string processGroupInner(std::string_view g,logSystem& logger);
  void notify(const ProgressEvent& event);
  std::string extractFldResultText(std::string_view fieldGroup);
  std::string cleanRtfControlWordsButKeepSemantic(std::string_view input);
};