// =====================================================
// Module : ConversionEngine (implementation)
// 偵測的目標必須用的容器是 std::vector<uint8_t> 才能真正判斷
// ansi 部份的編碼轉換使用外部函式庫 iconv 來達成
// =====================================================

#include "Decode.h"
#include "ErrorSystem_Module/ErrorSystem.h"
#include "ErrorDetector_Module/ErrorDetector.h"
#include "ErrorSystem_Module/ErrorHandle.h"
#include<optional>
#include<string>
#include<vector>
#include<cstdint>
#include<regex>
#include<cctype>
#include<cstddef>
#include<iconv.h>
#include<filesystem>


// UTF-8的解碼類別函式定義
std::optional<std::string> utf8Decoder::decode(const std::string& rtfContent,
                                    IByteErrorDetector& detector,
                                    ErrorHandle& errorhandler)
{
    std::vector<uint8_t> bytes = decodeUnToUint8_t(rtfContent);
    ErrorSystem::ErrorInfo info = detector.detect(bytes);
    
    if(!info.isEmpty()){
      errorhandler.handle(info);
      if(info.getMaxLevel() > ErrorSystem::ErrorLevel::Warning){
        return std::nullopt;
      }
    }
    std::string result(bytes.begin(),bytes.end());

    return result;
}
std::string utf8Decoder::encodeUTF8(int codepoint){
    std::string out;
    if (codepoint <= 0x7F) out.push_back((char)codepoint);
    else if (codepoint <= 0x7FF) {
        out.push_back(0xC0 | ((codepoint >> 6) & 0x1F));
        out.push_back(0x80 | (codepoint & 0x3F));
    } else if (codepoint <= 0xFFFF) {
        out.push_back(0xE0 | ((codepoint >> 12) & 0x0F));
        out.push_back(0x80 | ((codepoint >> 6) & 0x3F));
        out.push_back(0x80 | (codepoint & 0x3F));
    } else {
        out.push_back(0xF0 | ((codepoint >> 18) & 0x07));
        out.push_back(0x80 | ((codepoint >> 12) & 0x3F));
        out.push_back(0x80 | ((codepoint >> 6) & 0x3F));
        out.push_back(0x80 | (codepoint & 0x3F));
    }
    return out;
}
std::string utf8Decoder::decodeUnicodeAndClean(const std::string& content){
    std::string result;
    size_t i = 0;

    while(i<content.size()){
      if(content[i] == '\\' && i+2 < content.size() && content[i+1] == 'u' ){
        // 取得目標編碼後方的數字
        size_t j = i+2;
        bool negative = false;
        if(content[j] == '-'){ // 因為un編碼會有負號所以要特別處理
          negative = true;
          j++;
        }
        
        int value = 0;
        while(j < content.size() && isdigit(content[j])){
          value = value * 10 + (content[j] - '0');
          j++; 
        }
        if(negative) value = -value; // 有負號要轉換
        if(value < 0){
          value += 65536;
        }

        // 將數字依照轉換函式轉成 UTF-8
        result += encodeUTF8(value);

        /* \u19981 \'a4\'a3 */
        
        // 此段用來跳過ansi編碼訊息
        if(j+3 < content.size() && content[j] == '\\' && content[j+1] == '\''
           && isxdigit(content[j+2]) && isxdigit(content[j+3]))
        {
          j += 4;
          if(j+3 < content.size() && content[j] == '\\' && content[j+1] == '\''
           && isxdigit(content[j+2]) && isxdigit(content[j+3])) // 常常有兩個ansi 痕跡
           {
             j += 4;
           }  
        }

        i = j ;
      }else{// 不是 目標un碼 就不處理先放進字串 
        result.push_back(content[i]);
        i++;
      }
    }

    return result;
}
void utf8Decoder::encodeUTF8ToBytes(uint32_t codepoint,std::vector<uint8_t>& out){
  // Unicode 最大碼點檢查（安全保護）
    if (codepoint > 0x10FFFF) {
        // 你可以選擇：
        // 1) 忽略
        // 2) 丟 U+FFFD
        // 3) 回報錯誤
        // 這裡示範：替換為 U+FFFD
        out.push_back(0xEF);
        out.push_back(0xBF);
        out.push_back(0xBD);
        return;
    }

    // UTF-16 surrogate 範圍禁止
    if (codepoint >= 0xD800 && codepoint <= 0xDFFF) {
        // 同樣用 replacement character
        out.push_back(0xEF);
        out.push_back(0xBF);
        out.push_back(0xBD);
        return;
    }

    if (codepoint <= 0x7F) {
        // 1-byte: 0xxxxxxx
        out.push_back(static_cast<uint8_t>(codepoint));
    }else if (codepoint <= 0x7FF) {
        // 2-byte: 110xxxxx 10xxxxxx
        out.push_back(static_cast<uint8_t>(0xC0 | (codepoint >> 6)));
        out.push_back(static_cast<uint8_t>(0x80 | (codepoint & 0x3F)));
    }else if (codepoint <= 0xFFFF) {
        // 3-byte: 1110xxxx 10xxxxxx 10xxxxxx
        out.push_back(static_cast<uint8_t>(0xE0 | (codepoint >> 12)));
        out.push_back(static_cast<uint8_t>(0x80 | ((codepoint >> 6) & 0x3F)));
        out.push_back(static_cast<uint8_t>(0x80 | (codepoint & 0x3F)));
    }else {
        // 4-byte: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
        out.push_back(static_cast<uint8_t>(0xF0 | (codepoint >> 18)));
        out.push_back(static_cast<uint8_t>(0x80 | ((codepoint >> 12) & 0x3F)));
        out.push_back(static_cast<uint8_t>(0x80 | ((codepoint >> 6) & 0x3F)));
        out.push_back(static_cast<uint8_t>(0x80 | (codepoint & 0x3F)));
    }
}
std::vector<uint8_t> utf8Decoder::decodeUnToUint8_t(const std::string& content){
    std::vector<uint8_t> bytes;
    bytes.reserve(content.size() / 4);
    size_t ucSkip = 1;//RTF 規範為1

    for(size_t i =0;i<content.size();){
      
      // 找到 \uc控制符就更新狀態並跳過該控制符
      if(content[i] == '\\' && i+4<content.size() 
                            && content[i+1] == 'u'
                            && content[i+2] == 'c')
      {
        size_t j = i + 3;
        size_t value = 0;
        while (j < content.size() &&std::isdigit(static_cast<unsigned char>(content[j]))){
          value = value * 10 + (content[j] - '0');
          j++;
        }  
        ucSkip = value;
        i = j;
        continue;
      }
      
      // 處理 \un
      if(content[i] == '\\' && i+1 < content.size() && content[i+1] == 'u'){
        size_t j = i + 2;
        bool negative = false;
        
        // 用來抓有無負號
        if(j < content.size() && content[j] == '-'){
          negative = true;
          j++;
        }
        
        // 抓出\uN 的實際數字
        int value = 0;
        while(j < content.size() && std::isdigit(static_cast<unsigned char>(content[j]))){
          value = value * 10 + (content[j] - '0');
          j++;
        }

        if(negative) value = -value; // 有抓到負號要加上
        if(value < 0) value += 65536; // 規定
        
        encodeUTF8ToBytes(value,bytes);

        //用來忽略掉放在 \uN 後方的 \'XX
        //使用得到的 \uc 數值來決定執行次數
        for(size_t c = 0;c < ucSkip;c++){
          if (j + 3 < content.size() &&
            content[j] == '\\' && content[j + 1] == '\'' &&
            std::isxdigit(content[j + 2]) &&
            std::isxdigit(content[j + 3]))
          {
            j += 4;
          }else{
            break;
          }
        }
        
        i = j;
        continue;
      }
      
      // 遇到單獨存在的 \'XX  -> 轉成 raw byte
      if (content[i] == '\\' && i + 3 < content.size() &&
            content[i + 1] == '\'' &&
            std::isxdigit(content[i + 2]) &&
            std::isxdigit(content[i + 3])) 
      {
        uint8_t value = 0;
        auto hexToNibble = [](char c) -> uint8_t {
          if (c >= '0' && c <= '9') return c - '0';
          if (c >= 'a' && c <= 'f') return c - 'a' + 10;
          if (c >= 'A' && c <= 'F') return c - 'A' + 10;
          return 0;
        };

        value = (hexToNibble(content[i + 2]) << 4)
              |  hexToNibble(content[i + 3]);

        bytes.push_back(value);
        i += 4;
        continue;
      }
      
      // 其餘 ASCII 碼直接加入陣列中
      bytes.push_back(static_cast<uint8_t>(static_cast<unsigned char>(content[i])));
      i++;
    }

    return bytes;
}

// ANSI的解碼類別函式定義
ansiDecoder& ansiDecoder::setCodePage(int codepage){
    switch (codepage){
      case 950:
      fromCharset_ = "CP950";
      break;
      case 936:
      fromCharset_ = "GBK";
      break;
      case 932:
      fromCharset_ = "SHIFT_JIS";
      break;
      case 949:
      fromCharset_ = "CP949";
      break;
      case 1252:
      fromCharset_ = "CP1252";
      break;
      case 1250:
      fromCharset_ = "CP1250";
      break;
      case 1251:
      fromCharset_ = "CP1251";
      break;
      default:
      fromCharset_ = "CP1252"; // 預設英文
      break;
    }
    return *this;
}
std::optional<std::string> ansiDecoder::decode(const std::string& rtfContent,
                                               IByteErrorDetector& errordetector,
                                               ErrorHandle& errorhandle)
{
    if(rtfContent.empty()) return {}; //防止空白
    if(fromCharset_.empty()) return "need set codePage"; // 防止未設置 codepage
    
    std::vector<uint8_t> detectorProcessedBytes = DetectorHexToBytes_Scan(rtfContent,errorhandle);

    ErrorSystem::ErrorInfo info = errordetector.detect(detectorProcessedBytes);
    if(!info.isEmpty()){
      errorhandle.handle(info);
      if(info.getMaxLevel() > ErrorSystem::ErrorLevel::Warning){
        return std::nullopt; // 如果錯誤階級 大於警告就結束程序
      }
    }

    std::vector<uint8_t> processedBytes = DecodeHexToBytes(rtfContent);
    
    const char* toCharset = "UTF-8";

    iconv_t cd = iconv_open(toCharset,fromCharset_.c_str());
    if (cd == (iconv_t)-1) { // 防止開啟錯誤
      throw std::runtime_error("iconv_open failed: unsupported charset " + fromCharset_);
    }

    // input / output buffer 設定 (設定 iconv 所需參數)
    size_t inBytesLeft = processedBytes.size();
    const char* inBuf = reinterpret_cast<const char*>(processedBytes.data());// 取得陣列開頭的指標
    
    size_t outBufSize = inBytesLeft * 4;  // 預留 UTF-8 可能的最大長度
    std::string outStr(outBufSize, '\0');
    char* outBuf = outStr.data();
    size_t outBytesLeft = outBufSize;

    char* inBufPtr = const_cast<char*>(inBuf);
    char* outBufPtr = outBuf;

    
    // 使用 iconv 函式轉換,並依照其回傳值進行錯誤防呆
    size_t result = iconv(cd,&inBufPtr,&inBytesLeft, &outBufPtr, &outBytesLeft);
    if (result == (size_t)-1) { // 如果出現非預期行為
      iconv_close(cd);
      throw std::runtime_error("iconv conversion failed for " + fromCharset_);
    }
    
    iconv_close(cd);//用完隨手關掉
    outStr.resize(outBufSize - outBytesLeft);//用完將空間重預設的調整為實際的
    return outStr;
}
std::vector<uint8_t> ansiDecoder::DecodeHexToBytes(const std::string& rtfContent){
  // 所需參數
    std::vector<uint8_t> buffer;
    std::regex hexPattern(R"(\\'([0-9a-fA-F]{2}))");
    std::smatch match;
    auto it = rtfContent.cbegin();
    auto end = rtfContent.cend();

    while (std::regex_search(it, end, match, hexPattern)) {
      // 先把非 hex 的部分原樣保留
      buffer.insert(buffer.end(), it, match[0].first);

      // 把 \\'xx 轉成真實 byte
      unsigned int val = 0;
      std::stringstream ss;
      ss << std::hex << match[1].str();
      ss >> val;
      buffer.push_back(static_cast<uint8_t>(val));

      // 移動游標
      it = match.suffix().first;
    }

    // 把最後未匹配的部分補上
    buffer.insert(buffer.end(), it, end);

    return buffer;
}
std::vector<uint8_t> ansiDecoder::DetectorHexToBytes(const std::string& rtfContent){
  // 所需參數
    std::vector<uint8_t> buffer;
    std::regex hexPattern(R"(\\'([0-9a-fA-F]{2}))");
    std::smatch match;
    auto it = rtfContent.cbegin();
    auto end = rtfContent.cend();

    while (std::regex_search(it, end, match, hexPattern)) {
      // 把 \\'xx 轉成真實 byte
      unsigned int val = 0;
      std::stringstream ss;
      ss << std::hex << match[1].str();
      ss >> val;
      buffer.push_back(static_cast<uint8_t>(val));

      // 移動游標
      it = match.suffix().first;
    }
    
    return buffer;
}
inline int ansiDecoder::hexVal(char c){
    if (c >= '0' && c <= '9') return c - '0';
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    return -1;
}  
std::vector<uint8_t> ansiDecoder::DetectorHexToBytes_Scan(const std::string& rtfContent,ErrorHandle& errorhandle){
  std::vector<uint8_t> buffer;
    const size_t n = rtfContent.size();

    for(size_t i = 0; i < n;){
      if(rtfContent[i] == '\\' && i + 1 < n && rtfContent[i+1] == '\''){
        if(i+3 >= n){ // 邊界限制
          ErrorSystem::ErrorInfo info;
          info.add(ErrorSystem::ErrorType::Decode_ANSI_InvalidByte,
                   ErrorSystem::ErrorLevel::Warning,
                   ErrorSystem::ErrorCategory::Decode,
                   L"[基底編碼檢查過程錯誤(ANSI)] : 字串結尾處有錯誤 ANSI byte 開頭");
          errorhandle.handle(info);
          break;
        }
      
        int hi = hexVal(rtfContent[i+2]);
        int lo = hexVal(rtfContent[i+3]);
        if(hi >= 0 && lo >= 0){
          buffer.push_back(static_cast<uint8_t>((hi << 4) | lo));
          i += 4;
        }else{
          std::string errorStr = "\\\'";
          errorStr += rtfContent[i+2];
          errorStr += rtfContent[i+3];
          std::wstring wStr = std::filesystem::path(errorStr).wstring();// 取巧轉換 wstring
          ErrorSystem::ErrorInfo info;
          info.add(ErrorSystem::ErrorType::Decode_ANSI_InvalidByte,
                   ErrorSystem::ErrorLevel::Warning,
                   ErrorSystem::ErrorCategory::Decode,
                   L"[基底編碼檢查過程錯誤(ANSI)] : 偵測到錯誤 byte[ " + wStr + L" ],進行跳過處裡");
          errorhandle.handle(info);
          
          i += 4;
        }
        
        continue; 
      } 
      
      i++;
    }
    
    return buffer;
}