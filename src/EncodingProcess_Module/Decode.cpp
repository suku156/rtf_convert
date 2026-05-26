// =====================================================
// Module : ConversionEngine (implementation)
// 偵測的目標必須轉換成正確的byte(uint8_t)
// ansi 部份的編碼轉換使用外部函式庫 iconv 來達成
// =====================================================

#include "Decode.h"
#include "ErrorSystem_Module/ErrorSystem.h"
#include "ErrorDetector_Module/ErrorDetector.h"
#include "ErrorSystem_Module/ErrorHandle.h"
#include "Feedback_Module/ProgressEvent.h"
#include "Feedback_Module/IProgressObserver.h"
#include<optional>
#include<string>
#include<vector>
#include<cstdint>
#include<regex>
#include<cctype>
#include<cstddef>
#include<iconv.h>
#include<filesystem>

namespace{
  inline uint8_t hexToNibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return 0;
  }
}

// UTF-8的解碼類別函式定義
std::optional<std::string> utf8Decoder::decode(const std::string& rtfContent){
  std::string result = decodeUnToString(rtfContent);
  return result;
}

void utf8Decoder::appendCodepointAsUtf8(uint32_t codepoint, std::string& out) {
    auto pushByte = [&](uint8_t b) {
      out.push_back(static_cast<char>(b));
    };

    if (codepoint > 0x10FFFF ||
       (codepoint >= 0xD800 && codepoint <= 0xDFFF))
    {
      pushByte(0xEF);
      pushByte(0xBF);
      pushByte(0xBD);
      return;
    }

    if (codepoint <= 0x7F) {
      pushByte(static_cast<uint8_t>(codepoint));
    } else if (codepoint <= 0x7FF) {
      pushByte(static_cast<uint8_t>(0xC0 | (codepoint >> 6)));
      pushByte(static_cast<uint8_t>(0x80 | (codepoint & 0x3F)));
    } else if (codepoint <= 0xFFFF) {
      pushByte(static_cast<uint8_t>(0xE0 | (codepoint >> 12)));
      pushByte(static_cast<uint8_t>(0x80 | ((codepoint >> 6) & 0x3F)));
      pushByte(static_cast<uint8_t>(0x80 | (codepoint & 0x3F)));
    } else {
      pushByte(static_cast<uint8_t>(0xF0 | (codepoint >> 18)));
      pushByte(static_cast<uint8_t>(0x80 | ((codepoint >> 12) & 0x3F)));
      pushByte(static_cast<uint8_t>(0x80 | ((codepoint >> 6) & 0x3F)));
      pushByte(static_cast<uint8_t>(0x80 | (codepoint & 0x3F)));
    }
}

std::string utf8Decoder::decodeUnToString(const std::string& content) {
  std::string result;
  result.reserve(content.size());

  size_t ucSkip = 1; // RTF default

  for (size_t i = 0; i < content.size();) {

    // 處理 \ucN
    if (content[i] == '\\' &&
        i + 3 < content.size() &&
        content[i + 1] == 'u' &&
        content[i + 2] == 'c')
    {
      size_t j = i + 3;
      size_t value = 0;
      bool hasDigit = false;

      while (j < content.size() &&
              std::isdigit(static_cast<unsigned char>(content[j])))
      {
        hasDigit = true;
        value = value * 10 + (content[j] - '0');
        ++j;
      }

      if (hasDigit) {
        ucSkip = value;
        // 這裡選擇移除 \ucN，因為它只是 Unicode fallback 設定
        i = j;
        continue;
      }
    }

    // 處理 \uN
    if (content[i] == '\\' &&
        i + 1 < content.size() &&
        content[i + 1] == 'u')
    {
      size_t j = i + 2;
      bool negative = false;

      if (j < content.size() && content[j] == '-') {
          negative = true;
          ++j;
      }

      int value = 0;
      bool hasDigit = false;

      while (j < content.size() &&
            std::isdigit(static_cast<unsigned char>(content[j])))
      {
        hasDigit = true;
        value = value * 10 + (content[j] - '0');
        ++j;
      }

      if (hasDigit) {
        if (negative) value = -value;
        if (value < 0) value += 65536;

        appendCodepointAsUtf8(static_cast<uint32_t>(value), result);

        if (j < content.size() && content[j] == ' ') {
          ++j;
        }

        // 跳過 \uN 後面的 fallback
        for (size_t c = 0; c < ucSkip && j < content.size(); ++c) {
          if (j + 3 < content.size() &&
              content[j] == '\\' &&
              content[j + 1] == '\'' &&
              std::isxdigit(static_cast<unsigned char>(content[j + 2])) &&
              std::isxdigit(static_cast<unsigned char>(content[j + 3])))
          {
            j += 4;
          }else {
            ++j;
          }
        }

          i = j;
          continue;
        }
    }

      // 重點：不要在這裡轉 \'XX
      // 保留給下一階段 AnsiSegmentScanner + iconv
      result.push_back(content[i]);
      ++i;
  }

  return result;
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
AnsiDecoderResult ansiDecoder::decode(const std::string& rtfContent,
                                      AnsiErrorDetector& errordetector,
                                      ErrorHandle& errorhandle)
{
  AnsiDecoderResult AnsiDecoderresult;
  AnsiDecoderresult.state = decodeState::SuccessClean;
  // 防止空白
  if(rtfContent.empty()){
    AnsiDecoderresult.state = decodeState::SuccessClean;
    AnsiDecoderresult.str = "";
    return AnsiDecoderresult;
  }
  // 防止未設置 codepage
  if(fromCharset_.empty()){
    AnsiDecoderresult.state = decodeState::Failed;
    AnsiDecoderresult.str = std::nullopt;
    return AnsiDecoderresult;
  } 
  
  // 找到 \'XX 區域並放入特製的容器中
  std::vector<AnsiSymbolPending> result = FindHexToContainer(rtfContent);

  // 如果沒有找到 \'XX 區塊就直接回傳
  if(result.empty()){
    AnsiDecoderresult.state = decodeState::SuccessClean;
    AnsiDecoderresult.str = rtfContent;
    return AnsiDecoderresult;
  }

  // 用 iconv 對特製區進行處理,如果iconv回傳錯誤嘗試偵測並記錄
  AnsiDecoderresult.state =  ProcessHexContainer(result,errordetector,errorhandle);

  // 使用容器資訊替換舊字串 \'XX 區域內容,產生新字串
  AnsiDecoderresult.str = ApplyHexContainer(rtfContent,result);
  return AnsiDecoderresult;
}

std::vector<AnsiSymbolPending> ansiDecoder::FindHexToContainer(const std::string& rtfContent){
  std::vector<AnsiSymbolPending> result;
  for(size_t i = 0;i<rtfContent.size();){
    
    // 找到 \'XX 區域開頭
    if(rtfContent[i] == '\\' &&
       i + 3 < rtfContent.size() &&
       rtfContent[i + 1] == '\'' &&
       std::isxdigit(static_cast<unsigned char>(rtfContent[i + 2])) &&
       std::isxdigit(static_cast<unsigned char>(rtfContent[i + 2])))
    {
      AnsiSymbolPending pending;
      
      pending.start = i;

      // 收集連續的 \'XX 轉成 byte 放入容器中
      while(i + 3 < rtfContent.size() &&
            rtfContent[i] == '\\' &&
            rtfContent[i + 1] == '\'' &&
            std::isxdigit(static_cast<unsigned char>(rtfContent[i + 2])) &&
            std::isxdigit(static_cast<unsigned char>(rtfContent[i + 3])))
      {
        uint8_t value =(hexToNibble(rtfContent[i + 2]) << 4) |
                        hexToNibble(rtfContent[i + 3]);

        pending.bytes.push_back(value);

        i += 4;
      }

      pending.end = i;
      result.push_back(std::move(pending));
      continue;
    }

    i++;
  }

  return result;
}

std::optional<std::string> ansiDecoder::iconvToUtf8(const std::vector<uint8_t>& bytes){
  // 再檢查一次有無設置 codepage
  if(fromCharset_.empty()) return std::nullopt;

  if(bytes.empty()){
    return std::string{};
  }

  const char* toCharset = "UTF-8";
  iconv_t cd = iconv_open(toCharset,fromCharset_.c_str());
  if(cd == reinterpret_cast<iconv_t>(-1)){
    return std::nullopt;
  }

  std::string output;
  output.resize(bytes.size() * 4 + 16);

  char* inBuf = reinterpret_cast<char*>(
    const_cast<uint8_t*>(bytes.data())
  );
  size_t inBytesLeft = bytes.size();

  char* outBuf = output.data();
  size_t outBytesLeft = output.size();

  while (inBytesLeft > 0) {
    size_t ret = iconv(
      cd,
      &inBuf,
      &inBytesLeft,
      &outBuf,
      &outBytesLeft
    );

    if (ret == static_cast<size_t>(-1)) {
      iconv_close(cd);
      return std::nullopt;
    }
  }

  output.resize(output.size() - outBytesLeft);

  iconv_close(cd);
  return output;
}

decodeState ansiDecoder::ProcessHexContainer(std::vector<AnsiSymbolPending>& result,
                                             AnsiErrorDetector& errordetector,
                                             ErrorHandle& errorhandle)
{
  bool firstError = true;
  decodeState state = decodeState::SuccessClean;
  for(AnsiSymbolPending& item: result){
    std::optional<std::string> decoded = iconvToUtf8(item.bytes);
    if(!decoded){
      state = decodeState::SuccessWithWarning;
      if(firstError){
        notify(ProgressEvent{
          ProgressEventType::Warning,
          L"ANSI 解碼時發現異常片段，已保留原始內容，詳細資訊請查看 log"
        });
        firstError = false;
      }
      item.status = AnsiDecodeStatus::IconvFailed;
      item.text = std::nullopt;

      ErrorSystem::ErrorInfo info = errordetector.detect(item.bytes);
      if(!info.isEmpty()){
        errorhandle.handle(info);
      }
      continue;
    }
    
    item.status = AnsiDecodeStatus::Success;
    item.text = std::move(*decoded);
  }

  return state;
}

std::string ansiDecoder::ApplyHexContainer(const std::string& rtfContent,
                                           const std::vector<AnsiSymbolPending>& result)
{
  std::string output;
  output.reserve(rtfContent.size());

  size_t pos = 0;
  for (const auto& item : result) {
    // 先吃入目標區段前的普通內容
    if (pos < item.start) {
        output.append(rtfContent, pos, item.start - pos);
    }

    // 如果解碼成功，就放入解碼結果
    if (item.text.has_value()) {
      output.append(*item.text);
    } else {
      // 解碼失敗：保留原始 \'XX 區段
      output.append(rtfContent, item.start, item.end - item.start);
    }

    pos = item.end;
  }

  // 補上最後剩餘內容
  if (pos < rtfContent.size()) {
    output.append(rtfContent, pos, std::string::npos);
  }

  return output;
}
void ansiDecoder::notify(const ProgressEvent& event){
  if(observer_){
    observer_->onEvent(event);
  }
}