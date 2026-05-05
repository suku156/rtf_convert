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
// =====================================================
#pragma once
#include<string>
#include<string_view>
#include<cstddef>


// forowrd delecration
class logSystem;

class textRtfProcessor{
  enum class GroupDecision{
    Drop,
    KeepText,
    KeepAll
  };
public:
  void Processor(std::string& Cleaned,logSystem& logger);
  std::string removeIgnorableDestinations(std::string_view rtf);
private:
  inline bool checkAsciiScope(const std::string& text, size_t pos ,size_t len,bool tailOnly = false);
  inline bool isControlCharacter(char c);
  bool start_with(std::string_view s,const char* prefix);
  bool isKnownDroppableGroup(std::string_view g);
  inline bool isHex(char c);
  bool hasVisibleText(std::string_view g);
  GroupDecision classifyGroup(std::string_view group);
  void controlGroupProcessor(std::string& Cleaned);
  void pardPartClean(std::string& Cleaned);
  void controlSymbolChange(std::string& Cleaned);
  void finalRtfSymbolClean(std::string& Cleaned);
  void compressBlankLines(std::string& Cleaned,int maxBlankLines);
  void removeOuterBraces(std::string& Cleaned);
  void trimLeadingNewlines(std::string& Cleaned);
  void removeProtectionSymbol(std::string& Cleaned);
  void replaceShapeGroupsWithImageMarkers(std::string& rtf);
  bool isSkippableRtfEscape(const std::string& s, size_t pos);
  void sheetSymbolChange(std::string& Cleaned);
  std::string processGroupInner(std::string_view g);
};