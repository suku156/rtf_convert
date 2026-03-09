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

// 使用 byte 的錯誤偵測系統介面
class IByteErrorDetector{
public:
  virtual  ~IByteErrorDetector() = default;
  virtual ErrorSystem::ErrorInfo detect(const std::vector<uint8_t>& bytes) = 0;
};

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
  void checkControlWord(const std::string& content , ErrorSystem::ErrorInfo& info);
  void checkHexEscape(const std::string& content , ErrorSystem::ErrorInfo& info);
  void checkIllegalBackslash(const std::string& content,ErrorSystem::ErrorInfo& info);
  bool checkPictResidual(const std::string& content);
};

// utf8 編碼專用的錯誤偵測
class Utf8ErrorDetector : public IByteErrorDetector{
  ErrorSystem::ErrorInfo detect(const std::vector<uint8_t>& bytes)override;
};

// ansi 編碼專用的錯誤偵測
class AnsiErrorDetector : public IByteErrorDetector{
  Encoding enc_ = Encoding::Invalid;
public:
  AnsiErrorDetector(Encoding enc) : enc_(enc) {}
  ErrorSystem::ErrorInfo detect(const std::vector<uint8_t>& bytes)override;
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
};