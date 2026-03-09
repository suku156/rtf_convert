// =====================================================
// Module  : ErrorDetector
// Author  : suku156
// Purpose : 對 ansi(/'XX) 與 utf(/uXXXX) 格式將其解碼成人可識別文字
// Layer   : errordetect
//
// Depend  :
//   ErrorDetector
//   ErrorSystem
//   ErrorHandle  
// Used by :
//   RTFProcessor   
//
// Notes :
//   將檔案依照基礎編碼格式的控制符轉換為人眼可識別的文字(中文)
//   有導入 錯誤偵測 與 錯誤處理系統.會進行錯誤偵測並且進行記錄
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