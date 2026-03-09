// =====================================================
// Module  : Decode
// Author  : suku156
// Purpose : 對 ansi(/'XX) 與 utf(/uXXXX) 格式將其解碼成人可識別文字
// Layer   : Decode
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
#include<optional>
#include<string>
#include<vector>
#include<cstdint>

//forward declaration
class IByteErrorDetector;
class ErrorHandle;

// 解碼用的基底類別(介面)
class ITextDecode{
public:
  virtual ~ITextDecode() = default;
  virtual std::optional<std::string> decode(const std::string& rtfContent,
                                            IByteErrorDetector& detector,
                                            ErrorHandle& errorhandler) = 0; 
};

// UTF-8的解碼類別
class utf8Decoder : public ITextDecode{
public:
  std::optional<std::string> decode(const std::string& rtfContent,
                                    IByteErrorDetector& detector,
                                    ErrorHandle& errorhandler) override;
private:
  std::string encodeUTF8(int codepoint);
  std::string decodeUnicodeAndClean(const std::string& content);
  void encodeUTF8ToBytes(uint32_t codepoint,std::vector<uint8_t>& out);
  std::vector<uint8_t> decodeUnToUint8_t(const std::string& content);
};

// ANSI的解碼類別
class ansiDecoder : public ITextDecode{
  std::string fromCharset_;
public:
  ansiDecoder& setCodePage(int codepage);
  std::optional<std::string> decode(const std::string& rtfContent,
                                    IByteErrorDetector& errordetector,
                                    ErrorHandle& errorhandle)override;
private:
  std::vector<uint8_t> DecodeHexToBytes(const std::string& rtfContent);
  // regex 版本
  std::vector<uint8_t> DetectorHexToBytes(const std::string& rtfContent);
  static inline int hexVal(char c);
  std::vector<uint8_t> DetectorHexToBytes_Scan(const std::string& rtfContent,
                                               ErrorHandle& errorhandle);
};