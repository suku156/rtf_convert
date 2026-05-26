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
class Utf8ErrorDetector;
class AnsiErrorDetector; 
class ErrorHandle;
class IProgressObserver;
struct ProgressEvent;

// UTF-8的解碼類別
class utf8Decoder{
public:
  std::optional<std::string> decode(const std::string& rtfContent);
private:
  void appendCodepointAsUtf8(uint32_t codepoint, std::string& out);
  std::string decodeUnToString(const std::string& content);
};

enum class AnsiDecodeStatus {
  Pending,
  Success,
  IconvFailed,
  Utf8CheckFailed
}; 

// 擷取 ansi 控制符的結構
struct AnsiSymbolPending{
  size_t start = 0;
  size_t end   = 0;
  std::vector<uint8_t> bytes;
  std::optional<std::string> text;
  AnsiDecodeStatus status = AnsiDecodeStatus::Pending;
};

enum class decodeState{
  SuccessClean,
  SuccessWithWarning,
  Failed
};

struct AnsiDecoderResult{
  decodeState state;
  std::optional<std::string> str;
};

// ANSI的解碼類別
class ansiDecoder{
  std::string fromCharset_;
  IProgressObserver* observer_ = nullptr;
public:
  explicit ansiDecoder(IProgressObserver* observer = nullptr) : observer_(observer) {}
  ansiDecoder& setCodePage(int codepage);
  AnsiDecoderResult decode(const std::string& rtfContent,
                           AnsiErrorDetector& errordetector,
                           ErrorHandle& errorhandle);
private:
  std::vector<AnsiSymbolPending> FindHexToContainer(const std::string& rtfContent);
  std::optional<std::string> iconvToUtf8(const std::vector<uint8_t>& bytes);
  decodeState ProcessHexContainer(std::vector<AnsiSymbolPending>& result,
                                  AnsiErrorDetector& errordetector,
                                  ErrorHandle& errorhandle);
  std::string ApplyHexContainer(const std::string& rtfContent,
                                const std::vector<AnsiSymbolPending>& result);
  void notify(const ProgressEvent& event);
};