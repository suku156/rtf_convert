// =====================================================
// Module : ErrorDetector (implementation)
// =====================================================
#include "ErrorDetector.h"
#include "ErrorSystem_Module/ErrorSystem.h"
#include "Universal_Module/CommonEnum.h"
#include<vector>
#include<cstdint>
#include<string>
#include<string_view>
#include<iostream>
#include<iomanip>
#include<sstream>
#include<algorithm>


// RTF 文法層面函式定義
ErrorSystem::ErrorInfo GeneralErrorDetector::detect(const std::string& content){
  
    ErrorSystem::ErrorInfo info;
    
    if(!checkBraceBalance(content)){
      info.add(ErrorSystem::ErrorType::General_Warning,
               ErrorSystem::ErrorLevel::Warning,
               ErrorSystem::ErrorCategory::detector,
               L"[輸出品質檢查警告]: 大括號不平衡");
    }

    checkResidualControlWords(content,info);
    
    
    return info;
}
bool GeneralErrorDetector::checkBraceBalance(const std::string& content){
    int depth = 0;
    for (size_t i = 0; i < content.size(); ++i) {
      char c = content[i];

      if (c == '\\') {
        if (i + 1 < content.size()) {
          char next = content[i + 1];

          if (next == '{' || next == '}' || next == '\\') {
            ++i;        // 跳過 escaped brace
            continue;
          }
        }
      }

      if (c == '{') {
          ++depth;
      } else if (c == '}') {
          --depth;
          if (depth < 0){
            return false;
          } 
      }
    }

    return depth == 0;
}
void GeneralErrorDetector::checkResidualControlWords(
     const std::string& text,ErrorSystem::ErrorInfo& info)
{
  bool reported = false;

  auto addWarningOnce = [&](){
    if(!reported){
      info.add(
        ErrorSystem::ErrorType::General_Warning,
        ErrorSystem::ErrorLevel::Recoverable,
        ErrorSystem::ErrorCategory::detector,
        L"[輸出品質檢查警告]: 最終文字仍殘留疑似 RTF 控制符，可能有未完全清理的區塊"
      );
      reported = true;
    }
  };

  auto isHex = [](unsigned char c){
    return std::isxdigit(c) != 0;
  };

    for(size_t i = 0; i < text.size(); ++i){

        if(text[i] != '\\'){
            continue;
        }

        if(i + 1 >= text.size()){
            addWarningOnce();
            return;
        }

        char next = text[i + 1];

        // escaped backslash "\\"，可能是一般文字，不警告
        if(next == '\\'){
            ++i;
            continue;
        }

        // 殘留 ANSI hex escape: \'XX
        if(next == '\'')
        {
          addWarningOnce();
          return;
        }

        // 殘留常見 RTF 控制字
        static const std::vector<std::string> suspiciousWords = {
            "par", "line", "tab",
            "u", "uc",
            "rtlch", "ltrch",
            "hich", "dbch", "loch",
            "fcs", "insrsid",
            "fs", "f", "cf",
            "pard", "plain",
            "trowd", "cell", "row", "intbl",
            "pict", "jpegblip", "pngblip", "wmetafile"
        };

        for(const auto& word : suspiciousWords){
            size_t start = i + 1;

            if(start + word.size() <= text.size() &&
               text.compare(start, word.size(), word) == 0)
            {
                size_t end = start + word.size();

                // 避免 \paradise 被誤判成 \par
                if(end >= text.size() ||
                   !std::isalpha(static_cast<unsigned char>(text[end])))
                {
                    addWarningOnce();
                    return;
                }
            }
        }
    }
}



// utf8 編碼專用函式定義
ErrorSystem::ErrorInfo Utf8ErrorDetector::detectText(std::string_view text){
  ErrorSystem::ErrorInfo info;

    auto toUint8t = [](char c) -> uint8_t{
      return static_cast<uint8_t>(static_cast<unsigned char>(c));
    };
    
    size_t n = text.size();

    size_t i = 0;
    while(i < n){
      uint8_t c = toUint8t(text[i]);
      
      // [1] ASCII → 永遠合法
      if (c < 0x80) {
        i++;
        continue;
      }

      size_t needed = 0;
      uint32_t codepoint = 0;
      bool errorOccurred = false;

      // [2] 起始 byte 判斷
      if ((c & 0xE0) == 0xC0) {
        needed = 2;
        codepoint = c & 0x1F;

        if (c < 0xC2) {
          info.add(ErrorSystem::ErrorType::Decode_UTF8_ByteError,
                   ErrorSystem::ErrorLevel::Recoverable,
                   ErrorSystem::ErrorCategory::Decode,
                   L"[UTF8解碼偵測錯誤]: 2-byte 開頭為禁止的開頭,會造成overlong" +
                   buildBytePreview(text, i, 3));
          errorOccurred = true;
        }
      }else if ((c & 0xF0) == 0xE0) {
        needed = 3;
        codepoint = c & 0x0F;
      }else if ((c & 0xF8) == 0xF0) {
        needed = 4;
        codepoint = c & 0x07;

        if (c > 0xF4) {
          info.add(ErrorSystem::ErrorType::Decode_UTF8_ByteError,
                   ErrorSystem::ErrorLevel::Recoverable,
                   ErrorSystem::ErrorCategory::Decode,
                   L"[UTF8解碼偵測錯誤]: 超過 4-byte 開頭允許之範圍" + 
                   buildBytePreview(text, i, 3));
          errorOccurred = true;
        }
      }else {
        info.add(ErrorSystem::ErrorType::Decode_UTF8_ByteError,
                 ErrorSystem::ErrorLevel::Recoverable,
                 ErrorSystem::ErrorCategory::Decode,
                 L"[UTF8解碼偵測錯誤]: 非法的Byte 開頭" +
                 buildBytePreview(text, i, 3));
        i++; // fallback
        continue;
      }

      // [3] byte 不足
      if (needed > n - i) {
        info.add(ErrorSystem::ErrorType::Decode_UTF8_ByteError,
                 ErrorSystem::ErrorLevel::Recoverable,
                 ErrorSystem::ErrorCategory::Decode,
                 L"[UTF8解碼偵測錯誤]: Byte 需要數量超過容器大小");
        i++; // fallback
        continue;
      }

      // [4] 檢查 continuation bytes
      for (size_t j = 1; j < needed; j++) {
            uint8_t cc = toUint8t(text[i + j]);
            if ((cc & 0xC0) != 0x80) {
              info.add(ErrorSystem::ErrorType::Decode_UTF8_ByteError,
                       ErrorSystem::ErrorLevel::Recoverable,
                       ErrorSystem::ErrorCategory::Decode,
                       L"[UTF8解碼偵測錯誤]: continuation byte 格式錯誤" + 
                       buildBytePreview(text, i + j, 3));
              errorOccurred = true;
              break;
            }
      }

      if (errorOccurred) {
        i++; // fallback
        continue;
      }

      // [5] 組 codepoint
      for (size_t j = 1; j < needed; j++) {
        codepoint = (codepoint << 6) | (toUint8t(text[i + j]) & 0x3F);
      }

      // [6] overlong check
      if ((needed == 2 && codepoint < 0x80) ||
          (needed == 3 && codepoint < 0x800) ||
          (needed == 4 && codepoint < 0x10000)) 
      {
        info.add(ErrorSystem::ErrorType::Decode_UTF8_ByteError,
                 ErrorSystem::ErrorLevel::Recoverable,
                 ErrorSystem::ErrorCategory::Decode,
                 L"[UTF8解碼偵測錯誤]: overlong 違反最短長度" +
                 buildBytePreview(text, i, 3));
        i++; // fallback
        continue;
      }

      // [7] Surrogate check
      if (0xD800 <= codepoint && codepoint <= 0xDFFF) {
        info.add(ErrorSystem::ErrorType::Decode_UTF8_ByteError,
                 ErrorSystem::ErrorLevel::Recoverable,
                 ErrorSystem::ErrorCategory::Decode,
                 L"[UTF8解碼偵測錯誤]: 並非正常的 Unicode codepoint 區間" +
                 buildBytePreview(text, i, 3));
        i++; // fallback
        continue;
      }

      // [8] 最大碼點
      if (codepoint > 0x10FFFF) {
        info.add(ErrorSystem::ErrorType::Decode_UTF8_ByteError,
                 ErrorSystem::ErrorLevel::Recoverable,
                 ErrorSystem::ErrorCategory::Decode,
                 L"[UTF8解碼偵測錯誤]: 超過 Unicode定義的最大碼點" +
                 buildBytePreview(text, i, 3));
        i++; // fallback
        continue;
      }

      // [9] 正常 → 跳過整個 UTF-8 字元
      i += needed;
    }
    
  return info;
}
std::wstring Utf8ErrorDetector::buildBytePreview(
    std::string_view text,
    size_t pos,
    size_t radius)
{
    auto toUint8t = [](char c) -> uint8_t {
        return static_cast<uint8_t>(
            static_cast<unsigned char>(c)
        );
    };

    std::wstringstream ws;

    if (text.empty()) {
        return L"[附近 bytes]: <empty>";
    }

    size_t start = (pos > radius) ? pos - radius : 0;
    size_t end = std::min(text.size(), pos + radius + 1);

    ws << L"[index=" << pos << L", 附近 bytes]: ";

    for (size_t i = start; i < end; ++i) {
        uint8_t b = toUint8t(text[i]);

        if (i == pos) {
            ws << L"<<";
        }

        ws << std::uppercase
           << std::hex
           << std::setw(2)
           << std::setfill(L'0')
           << static_cast<int>(b);

        if (i == pos) {
            ws << L">>";
        }

        ws << L' ';
    }

    return ws.str();
}


// ansi 編碼專用函式定義
ErrorSystem::ErrorInfo AnsiErrorDetector::detect(const std::vector<uint8_t>& bytes){
    ErrorSystem::ErrorInfo info;
    if(enc_ == Encoding::Invalid){
      info.add(ErrorSystem::ErrorType::Decode_ANSI_codeLacking,
               ErrorSystem::ErrorLevel::Recoverable,
               ErrorSystem::ErrorCategory::Decode,
               L"[基底編碼錯誤檢查過程錯誤(ANSI)] : 未設定 ANSI 基底編碼的 codepage 無法執行");
      return info;
    }

    switch(enc_){
      case Encoding::Ansi_Big5:
        return detectBig5(bytes);
      case Encoding::Ansi_GBK:
        return detectGBK(bytes);
      case Encoding::Ansi_JIS:
        return detectSJIS(bytes);
      case Encoding::Ansi_Korean:
        return detectKorean(bytes);
      case Encoding::Ansi_Latin_1:
        return detectLatin1(bytes);
      case Encoding::Ansi_CEI:
        return detectCEI(bytes);
      case Encoding::Ansi_Cyrillic:
        return detectCyrillic(bytes);
      default:
       info.add(ErrorSystem::ErrorType::Decode_ANSI_codeLacking,
                ErrorSystem::ErrorLevel::Recoverable,
                ErrorSystem::ErrorCategory::Decode,
                L"[基底編碼錯誤檢查過程錯誤(ANSI)] : 不支援的 codepage");
      return info;
    }

    return info;
}
bool AnsiErrorDetector::isBig5Lead(uint8_t b){
  return b >= 0x81 && b <= 0xFE;
}
bool AnsiErrorDetector::isGBKLead(uint8_t b){
  return b >= 0x81 && b <= 0xFE;
}
bool AnsiErrorDetector::isSJISLead(uint8_t b){
    return (b >= 0x81 && b <= 0x9F) ||
           (b >= 0xE0 && b <= 0xFC);
}
bool AnsiErrorDetector::isEUCKRLead(uint8_t b){
  return b >= 0xA1 && b <= 0xFE;
}
bool AnsiErrorDetector::isBig5Trail(uint8_t b){
  return (b >= 0x40 && b <= 0x7E) ||
         (b >= 0xA1 && b <= 0xFE);
}
bool AnsiErrorDetector::isGBKTrail(uint8_t b){
  return (b >= 0x40 && b <= 0xFE && b != 0x7F);
}
bool AnsiErrorDetector::isSJISTrail(uint8_t b){
  return (b >= 0x40 && b <= 0x7E) ||
         (b >= 0x80 && b <= 0xFC);
}
bool AnsiErrorDetector::isEUCKRTrail(uint8_t b){
  return (b >= 0xA1 && b <= 0xFE);
}
ErrorSystem::ErrorInfo AnsiErrorDetector::detectBig5(const std::vector<uint8_t>& bytes){
    ErrorSystem::ErrorInfo info;
    
    const size_t end = bytes.size();
    
    size_t i = 0;
    while(i < end){
      uint8_t lead = bytes[i];

      // ASCII 區(合法直接跳過)
      if(lead < 0x80){
        i++;
        continue;
      }

      // 非法的開頭 byte 範圍
      if(!isBig5Lead(lead)){
        std::wstring wstr = buildBytePreview(bytes,i);
        info.add(ErrorSystem::ErrorType::Decode_ANSI_InvalidByte,
                 ErrorSystem::ErrorLevel::Recoverable,
                 ErrorSystem::ErrorCategory::Decode,
                 L"[ANSI_Big5 錯誤偵測區錯誤]: 非法的 byte 開頭,錯誤周邊 byte: " + wstr );
        i++;
        continue;
      }
      
      // 檢查有無第二個byte (半字偵測)
      if(i + 1 >= end){
        info.add(ErrorSystem::ErrorType::Decode_ANSI_TruncatedLeadByte,
                 ErrorSystem::ErrorLevel::Recoverable,
                 ErrorSystem::ErrorCategory::Decode,
                 L"[ANSI_Big5 錯誤偵測區錯誤]: 尾端 byte 的 trail byte 超過檔案大小,具有不完整的雙位元組");
        break;
      }

      uint8_t trail = bytes[i+1];

      // 如果有第二個 byte 檢查其是否符合 Big5 規定
      if(!isBig5Trail(trail)){
        std::wstring wstr = buildBytePreview(bytes,i);
        info.add(ErrorSystem::ErrorType::Decode_ANSI_InvalidByte,
                 ErrorSystem::ErrorLevel::Recoverable,
                 ErrorSystem::ErrorCategory::Decode,
                 L"[ANSI_Big5 錯誤偵測區錯誤]: 非法的第二 byte ,錯誤周邊 byte: " + wstr);
        i++;
        continue;         
      }

      // 遇到合法的 byte 就跳過兩個
      i += 2;
    }

    return info;
}
ErrorSystem::ErrorInfo AnsiErrorDetector::detectGBK(const std::vector<uint8_t>& bytes){
    ErrorSystem::ErrorInfo info;
    const size_t End = bytes.size();
    
    size_t i = 0;
    while(i < End){
      uint8_t lead = bytes[i];

      // ASCII 區(合法直接跳過)
      if(lead < 0x80){
        i++;
        continue;
      }

      // 非法的開頭 byte 範圍
      if(!isGBKLead(lead)){
        std::wstring wstr = buildBytePreview(bytes,i);
        info.add(ErrorSystem::ErrorType::Decode_ANSI_InvalidByte,
                 ErrorSystem::ErrorLevel::Recoverable,
                 ErrorSystem::ErrorCategory::Decode,
                 L"[ANSI_GBK 錯誤偵測區錯誤]: 非法的 byte 開頭,錯誤周邊 byte: " + wstr);
        i++;
        continue;
      }

      // 檢查有無第二個byte (半字偵測)
      if(i + 1 >= End){
        info.add(ErrorSystem::ErrorType::Decode_ANSI_TruncatedLeadByte,
                 ErrorSystem::ErrorLevel::Recoverable,
                 ErrorSystem::ErrorCategory::Decode,
                 L"[ANSI_GBK 錯誤偵測區錯誤]: 尾端 byte 的 trail byte 超過檔案大小,具有不完整的雙位元組");
        break;
      }

      uint8_t trail = bytes[i+1];

      // 如果有第二個 byte 檢查其是否符合 Big5 規定
      if(!isGBKTrail(trail)){
        std::wstring wstr = buildBytePreview(bytes,i);
        info.add(ErrorSystem::ErrorType::Decode_ANSI_InvalidByte,
                 ErrorSystem::ErrorLevel::Recoverable,
                 ErrorSystem::ErrorCategory::Decode,
                 L"[ANSI_GBK 錯誤偵測區錯誤]: 非法的第二 byte,錯誤周邊 byte: " + wstr);
        i++;
        continue;         
      }

      // 遇到合法的 byte 就跳過兩個
      i += 2;
    }

    return info;
}
ErrorSystem::ErrorInfo AnsiErrorDetector::detectSJIS(const std::vector<uint8_t>& bytes){
    ErrorSystem::ErrorInfo info;
    const size_t End = bytes.size();
    size_t i = 0;

    while(i < End){
      uint8_t lead = bytes[i];

      // 1. ASCII 區(合法)
      if(lead < 0x80){
        i++;
        continue;
      }

      // 2. 半形片假名(JIS 獨有 合法單 byte)
      if(lead >= 0xA1 && lead <= 0xDF){
        i++;
        continue;
      }
      
      // 3.檢查是否為合法開頭 byte
      if(!isSJISLead(lead)){
        std::wstring wstr = buildBytePreview(bytes,i);
        info.add(ErrorSystem::ErrorType::Decode_ANSI_InvalidByte,
                 ErrorSystem::ErrorLevel::Recoverable,
                 ErrorSystem::ErrorCategory::Decode,
                 L"[ANSI_JIS 錯誤偵測區錯誤]: 非法的 byte 開頭,錯誤周邊 byte: " + wstr);
        i++;
        continue;
      }

      // 4. 檢查有無第二個byte (半字偵測)
      if(i + 1 >= End){
        info.add(ErrorSystem::ErrorType::Decode_ANSI_TruncatedLeadByte,
                 ErrorSystem::ErrorLevel::Recoverable,
                 ErrorSystem::ErrorCategory::Decode,
                 L"[ANSI_JIS 錯誤偵測區錯誤]: 尾端 byte 的 trail byte 超過檔案大小,具有不完整的雙位元組");
        break;
      }

      uint8_t trail = bytes[i+1];
      
      // 5. 檢查第二byte 的合法性
      if(!isSJISTrail(trail)){
        std::wstring wstr = buildBytePreview(bytes,i);
        info.add(ErrorSystem::ErrorType::Decode_ANSI_InvalidByte,
                 ErrorSystem::ErrorLevel::Recoverable,
                 ErrorSystem::ErrorCategory::Decode,
                 L"[ANSI_JIS 錯誤偵測區錯誤]: 非法的第二 byte,錯誤周邊 byte: " + wstr);
        i++;
        continue; 
      }

      i += 2;
    }

    return info;
}
ErrorSystem::ErrorInfo AnsiErrorDetector::detectKorean(const std::vector<uint8_t>& bytes){
    ErrorSystem::ErrorInfo info;
    const size_t End = bytes.size();
    
    size_t i = 0;
    while(i < End){
      uint8_t lead = bytes[i];

      // ASCII 區(合法直接跳過)
      if(lead < 0x80){
        i++;
        continue;
      }

      // 非法的開頭 byte 範圍
      if(!isEUCKRLead(lead)){
        std::wstring wstr = buildBytePreview(bytes,i);
        info.add(ErrorSystem::ErrorType::Decode_ANSI_InvalidByte,
                 ErrorSystem::ErrorLevel::Recoverable,
                 ErrorSystem::ErrorCategory::Decode,
                 L"[ANSI_Korean 錯誤偵測區錯誤]: 非法的 byte 開頭,錯誤周邊 byte: " + wstr);
        i++;
        continue;
      }

      // 檢查有無第二個byte (半字偵測)
      if(i + 1 >= End){
        info.add(ErrorSystem::ErrorType::Decode_ANSI_TruncatedLeadByte,
                 ErrorSystem::ErrorLevel::Recoverable,
                 ErrorSystem::ErrorCategory::Decode,
                 L"[ANSI_Korean 錯誤偵測區錯誤]: 尾端 byte 的 trail byte 超過檔案大小,具有不完整的雙位元組");
        break;
      }

      uint8_t trail = bytes[i+1];

      // 檢查第二位元是否符合 CP碼規定
      if(!isEUCKRTrail(trail)){
        std::wstring wstr = buildBytePreview(bytes,i);
        info.add(ErrorSystem::ErrorType::Decode_ANSI_InvalidByte,
                 ErrorSystem::ErrorLevel::Recoverable,
                 ErrorSystem::ErrorCategory::Decode,
                 L"[ANSI_Korean 錯誤偵測區錯誤]: 非法的第二 byte,錯誤周邊 byte: " + wstr);
        i++;
        continue;         
      }

      // 遇到合法的 byte 就跳過兩個
      i += 2;
    }

    return info;
}
ErrorSystem::ErrorInfo AnsiErrorDetector::detectLatin1(const std::vector<uint8_t>& bytes){
  ErrorSystem::ErrorInfo info;
    
    for(auto c : bytes){
      if(c == 0x00){
        info.add(ErrorSystem::ErrorType::Decode_ANSI_InvalidByte,
                 ErrorSystem::ErrorLevel::Recoverable,
                 ErrorSystem::ErrorCategory::Decode,
                 L"[ANSI_Latin_1 錯誤偵測區錯誤]: 出現 null byte(0x00)");
        break;
      }
    }

    return info;
}
ErrorSystem::ErrorInfo AnsiErrorDetector::detectCEI(const std::vector<uint8_t>& bytes){
    ErrorSystem::ErrorInfo info;
    
    for(auto c : bytes){
      if(c == 0x00){
        info.add(ErrorSystem::ErrorType::Decode_ANSI_InvalidByte,
                 ErrorSystem::ErrorLevel::Recoverable,
                 ErrorSystem::ErrorCategory::Decode,
                 L"[ANSI_CEI 錯誤偵測區錯誤]: 出現 null byte(0x00)");
        break;
      }
    }

    return info;  
}
ErrorSystem::ErrorInfo AnsiErrorDetector::detectCyrillic(const std::vector<uint8_t>& bytes){
  ErrorSystem::ErrorInfo info;
    
    for(auto c : bytes){
      if(c == 0x00){
        info.add(ErrorSystem::ErrorType::Decode_ANSI_InvalidByte,
                 ErrorSystem::ErrorLevel::Recoverable,
                 ErrorSystem::ErrorCategory::Decode,
                 L"[ANSI_Cyrillic 錯誤偵測區錯誤]: 出現 null byte(0x00)");
        break;
      }
    }

    return info; 
}
std::wstring AnsiErrorDetector::buildBytePreview(const std::vector<uint8_t>& bytes,
                                                 size_t pos,size_t radius)
{
  std::wstringstream ws;

  size_t start = (pos > radius) ? pos - radius : 0;
  size_t end =
      std::min(bytes.size(), pos + radius + 1);

  ws << L"[附近 bytes] ";

  for (size_t i = start; i < end; ++i) {

    if (i == pos) {
        ws << L"<<";
    }

    ws << std::uppercase
       << std::hex
       << std::setw(2)
       << std::setfill(L'0')
       << static_cast<int>(bytes[i]);

    if (i == pos) {
        ws << L">>";
    }

    ws << L' ';
  }

  return ws.str(); 
}

