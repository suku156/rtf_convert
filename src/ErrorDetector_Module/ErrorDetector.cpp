#include "ErrorDetector.h"
#include "ErrorSystem_Module/ErrorSystem.h"
#include "Universal_Module/CommonEnum.h"
#include<vector>
#include<cstdint>
#include<string>

// RTF 文法層面函式定義
ErrorSystem::ErrorInfo GeneralErrorDetector::detect(const std::string& content){
  
    ErrorSystem::ErrorInfo info;
    
    if(!checkBraceBalance(content)){
      info.add(ErrorSystem::ErrorType::General_Fatal,
               ErrorSystem::ErrorLevel::FatalLocal,
               ErrorSystem::ErrorCategory::Decode,
               L"[圖片處理後的純文字檢查錯誤]: 大括號不平衡");
    }
    
    checkControlWord(content,info); // 檢查控制符基本格式
    checkHexEscape(content,info);   // 檢查hex 是否合法 
    checkIllegalBackslash(content,info); //確保沒有無法判斷的控制符格式

    if(!checkPictResidual(content)){
      info.add(ErrorSystem::ErrorType::General_Warning,
               ErrorSystem::ErrorLevel::Warning,
               ErrorSystem::ErrorCategory::Decode,
               L"[圖片處理後的純文字檢查錯誤]: 偵測到可能有圖片區塊未清理乾淨");
    }

    return info;
}
bool GeneralErrorDetector::checkBraceBalance(const std::string& content){
    int depth = 0;
    for(char c : content){
      if(c == '{'){
        depth++;
      }else if(c == '}'){
        depth--;
        if(depth < 0){
          return false;
        }
      }
    }
    
    if(depth == 0){
      return true;
    }
    
    return false;
}
void GeneralErrorDetector::checkControlWord(const std::string& content , ErrorSystem::ErrorInfo& info){
    const size_t maxSize = content.size();
    size_t i = 0;

    auto isControlSymbol = [](char c){
      return c=='~' || c=='_' || c=='{' || c=='}' ||
             c=='-' || c=='|' || c==':' || c=='*'; 
    };

    while(i < maxSize){
      
      if(content[i] == '\\' && i+1 < maxSize && content[i+1] == '\\'){
        i += 2;
        continue;
      }
      
      if(content[i] != '\\'){
        i++;
        continue;
      }

      if(i+1 >= maxSize){
        info.add(ErrorSystem::ErrorType::General_Fatal,
                 ErrorSystem::ErrorLevel::FatalLocal,
                 ErrorSystem::ErrorCategory::Decode,
                 L"[圖片處理後的純文字檢查錯誤]: 發現字串結尾有孤立的控制符開頭 \\");
      }

      char next = content[i+1];

      if(isControlSymbol(next)){ // 合法控制符
        i += 2;
        continue;
      }

      if(std::isalpha(static_cast<unsigned char>(next))){
        size_t j = i+1;

        while(j < maxSize && std::isalpha(static_cast<unsigned char>(content[j]))) j++;
        while(j < maxSize && content[j] == '-') j++;
        while(j < maxSize && std::isdigit(static_cast<unsigned char>(content[j]))) j++;

        i = j;
        continue;
      }

      if(next == '\''){
        i++;
        continue;
      }
      
      info.add(ErrorSystem::ErrorType::General_Fatal,
                 ErrorSystem::ErrorLevel::FatalLocal,
                 ErrorSystem::ErrorCategory::Decode,
                 L"[圖片處理後的純文字檢查錯誤]: 存在無法解析的非法控制符");
      return;           
    }
}
void GeneralErrorDetector::checkHexEscape(const std::string& content , ErrorSystem::ErrorInfo& info){
    const size_t maxSize = content.size();
    size_t i = 0;

    auto isHex = [](char c){
      return (c >= '0' && c <= '9') ||
             (c >= 'A' && c <= 'F') ||
             (c >= 'a' && c <= 'f');
    };
    
    while (i < maxSize){
      if(content[i] == '\\' && i+3 < maxSize && content[i+1] == '\''){
        char h1 = content[i+2];
        char h2 = content[i+3];

        if (!isHex(h1) || !isHex(h2)){
          info.add(ErrorSystem::ErrorType::General_Fatal,
                   ErrorSystem::ErrorLevel::FatalLocal,
                   ErrorSystem::ErrorCategory::Decode,
                   L"[圖片處理後的純文字檢查錯誤]: 非法 hex escape");
          return;
        }

        i += 4;
        continue;
      }
      i++;
    }
}
void GeneralErrorDetector::checkIllegalBackslash(const std::string& content,ErrorSystem::ErrorInfo& info){
    size_t maxSize = content.size();
    for(size_t i =0;i < maxSize;i++){
        
        if(content[i] != '\\') continue;

        if(i+1 >= maxSize){
        info.add(ErrorSystem::ErrorType::General_Fatal,
                    ErrorSystem::ErrorLevel::FatalLocal,
                    ErrorSystem::ErrorCategory::Decode,
                    L"[圖片處理後的純文字檢查錯誤]: 非法的 孤立反斜線");
        return;
        }

        char next = content[i+1];

        if(next == '\\'){
        i += 1;
        continue;
        }

        if(next == '\'') continue;

        if(std::isalpha(static_cast<unsigned char>(next))) continue;

        if (next=='~' || next=='_' || next=='{' ||
            next=='}' || next=='-' || next=='|' ||
            next==':' || next=='*' || next=='n')
        {
        continue;
        }

        if(static_cast<unsigned char>(next) >= 0x80){
        info.add(ErrorSystem::ErrorType::General_Fatal,
                    ErrorSystem::ErrorLevel::FatalLocal,
                    ErrorSystem::ErrorCategory::Decode,
                    L"[圖片處理後的純文字檢查錯誤]: 控制符中含有高位字元");
        return;
        }

        info.add(ErrorSystem::ErrorType::General_Fatal,
                ErrorSystem::ErrorLevel::FatalLocal,
                ErrorSystem::ErrorCategory::Decode,
                L"[圖片處理後的純文字檢查錯誤]: 未知的控制符");
        return;
    }
}
bool GeneralErrorDetector::checkPictResidual(const std::string& content){
    size_t pos = content.find("\\pict");
    if(pos != std::string::npos){
      return false;
    }
    return true;
}

// utf8 編碼專用函式定義
ErrorSystem::ErrorInfo Utf8ErrorDetector::detect(const std::vector<uint8_t>& bytes){
    ErrorSystem::ErrorInfo info;
    
    size_t n = bytes.size();

    size_t i = 0;
    while(i < n){
      uint8_t c = bytes[i];
      
      // [1] ASCII → 永遠合法
      if (c < 0x80) {
        i++;
        continue;
      }

      size_t needed = 0;
      uint32_t codepoint = 0;
      bool errorOccured = false;

      // [2] 起始 byte 判斷
      if ((c & 0xE0) == 0xC0) {
        needed = 2;
        codepoint = c & 0x1F;

        if (c < 0xC2) {
          info.add(ErrorSystem::ErrorType::Decode_UTF8_ByteError,
                   ErrorSystem::ErrorLevel::Recoverable,
                   ErrorSystem::ErrorCategory::Decode,
                   L"[UTF8解碼偵測錯誤]: 2-byte 開頭為禁止的開頭,會造成overlong");
          errorOccured = true;
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
                   L"[UTF8解碼偵測錯誤]: 超過 4-byte 開頭允許之範圍");
          errorOccured = true;
        }
      }else {
        info.add(ErrorSystem::ErrorType::Decode_UTF8_ByteError,
                 ErrorSystem::ErrorLevel::Recoverable,
                 ErrorSystem::ErrorCategory::Decode,
                 L"[UTF8解碼偵測錯誤]: 非法的Byte 開頭");
        i++; // fallback
        continue;
      }

      // [3] byte 不足
      if (i + needed > n) {
        info.add(ErrorSystem::ErrorType::Decode_UTF8_ByteError,
                 ErrorSystem::ErrorLevel::Recoverable,
                 ErrorSystem::ErrorCategory::Decode,
                 L"[UTF8解碼偵測錯誤]: Byte 需要數量超過容器大小");
        i++; // fallback
        continue;
      }

      // [4] 檢查 continuation bytes
      for (size_t j = 1; j < needed; j++) {
            uint8_t cc = bytes[i + j];
            if ((cc & 0xC0) != 0x80) {
              info.add(ErrorSystem::ErrorType::Decode_UTF8_ByteError,
                       ErrorSystem::ErrorLevel::Recoverable,
                       ErrorSystem::ErrorCategory::Decode,
                       L"[UTF8解碼偵測錯誤]: continuation byte 格式錯誤");
              errorOccured = true;
              break;
            }
      }

      if (errorOccured) {
        i++; // fallback
        continue;
      }

      // [5] 組 codepoint
      for (size_t j = 1; j < needed; j++) {
        codepoint = (codepoint << 6) | (bytes[i + j] & 0x3F);
      }

      // [6] overlong check
      if ((needed == 2 && codepoint < 0x80) ||
          (needed == 3 && codepoint < 0x800) ||
          (needed == 4 && codepoint < 0x10000)) 
      {
        info.add(ErrorSystem::ErrorType::Decode_UTF8_ByteError,
                 ErrorSystem::ErrorLevel::Recoverable,
                 ErrorSystem::ErrorCategory::Decode,
                 L"[UTF8解碼偵測錯誤]: overlong 違反最短長度");
        i++; // fallback
        continue;
      }

      // [7] Surrogate check
      if (0xD800 <= codepoint && codepoint <= 0xDFFF) {
        info.add(ErrorSystem::ErrorType::Decode_UTF8_ByteError,
                 ErrorSystem::ErrorLevel::Recoverable,
                 ErrorSystem::ErrorCategory::Decode,
                 L"[UTF8解碼偵測錯誤]: 並非正常的 Unicode codepoint 區間");
        i++; // fallback
        continue;
      }

      // [8] 最大碼點
      if (codepoint > 0x10FFFF) {
        info.add(ErrorSystem::ErrorType::Decode_UTF8_ByteError,
                 ErrorSystem::ErrorLevel::Recoverable,
                 ErrorSystem::ErrorCategory::Decode,
                 L"[UTF8解碼偵測錯誤]: 超過 Unicode定義的最大碼點");
        i++; // fallback
        continue;
      }

      // [9] 正常 → 跳過整個 UTF-8 字元
      i += needed;
    }
    
    return info;
  
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
        info.add(ErrorSystem::ErrorType::Decode_ANSI_InvalidByte,
                 ErrorSystem::ErrorLevel::Recoverable,
                 ErrorSystem::ErrorCategory::Decode,
                 L"[ANSI_Big5 錯誤偵測區錯誤]: 非法的 byte 開頭");
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
        info.add(ErrorSystem::ErrorType::Decode_ANSI_InvalidByte,
                 ErrorSystem::ErrorLevel::Recoverable,
                 ErrorSystem::ErrorCategory::Decode,
                 L"[ANSI_Big5 錯誤偵測區錯誤]: 非法的第二 byte");
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
        info.add(ErrorSystem::ErrorType::Decode_ANSI_InvalidByte,
                 ErrorSystem::ErrorLevel::Recoverable,
                 ErrorSystem::ErrorCategory::Decode,
                 L"[ANSI_GBK 錯誤偵測區錯誤]: 非法的 byte 開頭");
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
        info.add(ErrorSystem::ErrorType::Decode_ANSI_InvalidByte,
                 ErrorSystem::ErrorLevel::Recoverable,
                 ErrorSystem::ErrorCategory::Decode,
                 L"[ANSI_GBK 錯誤偵測區錯誤]: 非法的第二 byte");
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
        info.add(ErrorSystem::ErrorType::Decode_ANSI_InvalidByte,
                 ErrorSystem::ErrorLevel::Recoverable,
                 ErrorSystem::ErrorCategory::Decode,
                 L"[ANSI_JIS 錯誤偵測區錯誤]: 非法的 byte 開頭");
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
        info.add(ErrorSystem::ErrorType::Decode_ANSI_InvalidByte,
                 ErrorSystem::ErrorLevel::Recoverable,
                 ErrorSystem::ErrorCategory::Decode,
                 L"[ANSI_JIS 錯誤偵測區錯誤]: 非法的第二 byte");
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
        info.add(ErrorSystem::ErrorType::Decode_ANSI_InvalidByte,
                 ErrorSystem::ErrorLevel::Recoverable,
                 ErrorSystem::ErrorCategory::Decode,
                 L"[ANSI_Korean 錯誤偵測區錯誤]: 非法的 byte 開頭");
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
        info.add(ErrorSystem::ErrorType::Decode_ANSI_InvalidByte,
                 ErrorSystem::ErrorLevel::Recoverable,
                 ErrorSystem::ErrorCategory::Decode,
                 L"[ANSI_Korean 錯誤偵測區錯誤]: 非法的第二 byte");
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