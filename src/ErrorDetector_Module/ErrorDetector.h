// =====================================================
// Module  : ErrorDetector
// Author  : suku156
// Purpose : 負責偵測錯誤
// Layer   : errordetect
//
// Depend  :
//   CommonEnum
//   ErrorSystem
//
// Used by :
//   RTFProcessor   
//   Decode
// Notes :
//   負責錯誤偵測的模組,目前有 文法,ansi解碼以及utf解碼三種
//   
// =====================================================
#pragma once
#include "ErrorSystem_Module/ErrorSystem.h"
#include "Universal_Module/CommonEnum.h"
#include<vector>
#include<cstdint>
#include<string>
#include<string_view>


// 使用 text 的錯誤偵測系統介面
class ITextErrorDetector{
public:
  virtual  ~ITextErrorDetector() = default;
  virtual ErrorSystem::ErrorInfo detect(const std::string& content) = 0;
};

// RTF 文法層面用的錯誤偵測
class GeneralErrorDetector : public ITextErrorDetector{
public:
  ErrorSystem::ErrorInfo detect(const std::string& content) override;
private:
  bool checkBraceBalance(const std::string& content);
  void checkResidualControlWords(const std::string& text,ErrorSystem::ErrorInfo& info);
};

// utf8 編碼專用的錯誤偵測
class Utf8ErrorDetector {
public:
  ErrorSystem::ErrorInfo detectText(std::string_view text);
  std::wstring buildBytePreview(std::string_view text,size_t pos,size_t radius);
};

// ansi 編碼專用的錯誤偵測
class AnsiErrorDetector{
  Encoding enc_ = Encoding::Invalid;
public:
  AnsiErrorDetector(Encoding enc) : enc_(enc) {}
  ErrorSystem::ErrorInfo detect(const std::vector<uint8_t>& bytes);
private:
  bool isBig5Lead(uint8_t b);
  bool isGBKLead(uint8_t b);
  bool isSJISLead(uint8_t b);
  bool isEUCKRLead(uint8_t b);
  bool isBig5Trail(uint8_t b);
  bool isGBKTrail(uint8_t b);
  bool isSJISTrail(uint8_t b);
  bool isEUCKRTrail(uint8_t b);
  ErrorSystem::ErrorInfo detectBig5(const std::vector<uint8_t>& bytes);
  ErrorSystem::ErrorInfo detectGBK(const std::vector<uint8_t>& bytes);
  ErrorSystem::ErrorInfo detectSJIS(const std::vector<uint8_t>& bytes);
  ErrorSystem::ErrorInfo detectKorean(const std::vector<uint8_t>& bytes);
  ErrorSystem::ErrorInfo detectLatin1(const std::vector<uint8_t>& bytes);
  ErrorSystem::ErrorInfo detectCEI(const std::vector<uint8_t>& bytes);
  ErrorSystem::ErrorInfo detectCyrillic(const std::vector<uint8_t>& bytes);
  std::wstring buildBytePreview(const std::vector<uint8_t>& bytes,
                                size_t pos,size_t radius = 3);
};