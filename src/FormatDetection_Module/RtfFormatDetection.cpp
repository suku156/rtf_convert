// =====================================================
// Module : RtfFormatDetection (implementation)
// =====================================================
#include "RtfFormatDetection.h"
#include "Universal_Module/CommonEnum.h"
#include<fstream>
#include<vector>
#include<cstdint>
#include<string>
#include<string_view>
#include<sstream>
#include<array>
#include<charconv>
#include<cctype>
#include<optional> 


SizeProbeInfo EncodingDetector::isFileTooShort(std::ifstream& input,std::size_t minSize){
    SizeProbeInfo info;
    info.minRequiredSize = minSize;

    input.clear();
    auto oldPos = input.tellg();

    input.seekg(0, std::ios::end);
    auto endPos = input.tellg();

    input.clear();
    input.seekg(oldPos);

    if (endPos < 0) {
        info.isTooShort = true;
        return info;
    }

    info.fileSize = static_cast<std::size_t>(endPos);
    info.isTooShort = info.fileSize < minSize;

    return info;
}

BomType EncodingDetector::detectBom(std::ifstream& input) {
    input.clear();
    input.seekg(0, std::ios::beg);

    std::array<unsigned char, 4> buf{};
    input.read(reinterpret_cast<char*>(buf.data()), buf.size());
    const auto n = input.gcount();

    input.clear();
    input.seekg(0, std::ios::beg);

    if (n >= 4 &&
        buf[0] == 0xFF && buf[1] == 0xFE &&
        buf[2] == 0x00 && buf[3] == 0x00)
        return BomType::Utf32LE;

    if (n >= 4 &&
        buf[0] == 0x00 && buf[1] == 0x00 &&
        buf[2] == 0xFE && buf[3] == 0xFF)
        return BomType::Utf32BE;

    if (n >= 3 &&
        buf[0] == 0xEF && buf[1] == 0xBB && buf[2] == 0xBF)
        return BomType::Utf8;

    if (n >= 2 &&
        buf[0] == 0xFF && buf[1] == 0xFE)
        return BomType::Utf16LE;

    if (n >= 2 &&
        buf[0] == 0xFE && buf[1] == 0xFF)
        return BomType::Utf16BE;

    return BomType::None;
}

bool EncodingDetector::rtfSymbolCheck(std::string_view strView){
  size_t pos = strView.find('{');
  if(pos == std::string_view::npos) return false;
  
  size_t j = pos + 1;
  
  while(j < strView.size() &&
        std::isspace(static_cast<unsigned char>(strView[j])))
  {
    j++;
  }

  // root group 第一個有效 token 必須是 '\'
  if (j >= strView.size() || strView[j] != '\\') {
    return false;
  }

  ++j; // skip '\'

  // control word 必須是 rtf
  if (j + 3 > strView.size() ||
    strView.substr(j, 3) != "rtf") {
    return false;
  }

  j += 3; // 跳過 rtf

   // \rtf 後面最好要有版本數字，例如 \rtf1
  if (j >= strView.size() ||
    !std::isdigit(static_cast<unsigned char>(strView[j]))) {
    return false;
  }

  while (j < strView.size() &&
         std::isdigit(static_cast<unsigned char>(strView[j]))) {
    ++j;
  }

  return true;
}

std::optional<int> EncodingDetector::ansiCodePageCheck(std::string_view strView){
  constexpr std::string_view target = "\\ansicpg";
  
  size_t pos = strView.find(target);
  if(pos == std::string_view::npos){
    return std::nullopt;
  }
  size_t numStart = pos + target.size();
  size_t numEnd   = numStart;
  while(numEnd < strView.size() &&
        std::isdigit(static_cast<unsigned char>(strView[numEnd])))
  {
    numEnd++;
  }

  if (numStart == numEnd) {
    return std::nullopt;
  }

  int codePage = 0;

  auto result = std::from_chars(
    strView.data() + numStart,
    strView.data() + numEnd,
    codePage
  );

  if (result.ec != std::errc{}) {
    return std::nullopt;
  }

  return codePage;
}

// 類別 : Facade 模式,函式定義
DetectOutputNew EncodingDetector::detectEncoding(std::ifstream& input){
    
    struct DetectOutputNew newResult;
    
    // 基礎長度檢查不符合就判定為過短不可能當作正常 RTF 檔案處理
    newResult.sizeInfo = isFileTooShort(input, 7);
      
    // 找看看有無 BOM 儲存其資訊
    newResult.bomtype = detectBom(input);

    // 後續轉化為字串處裡
    input.clear();
    input.seekg(0, std::ios::beg);
    std::string content{
      std::istreambuf_iterator<char>(input),
      std::istreambuf_iterator<char>()
    }; 
    std::string_view strView(content.data(),content.size());

    // 檢查開頭 {\rtf 是否合規
    newResult.hasValidRtfRootHeader = rtfSymbolCheck(strView);

    // 偵測有無 ansicpg
    newResult.ansiCodePage = ansiCodePageCheck(strView);

    return newResult;
    
    /*
    DetectorBom bom;
    DetectorAnsi ansi;
    DetectorNoBom nobom;

    //依照預定順序檢查
    bom.readData(input);
    auto enc = bom.Detect();
    if(enc.result   != DetectorResult::Invalid &&
       enc.encoding != Encoding::Invalid) return enc;

    ansi.readData(input);
    enc = ansi.Detect();
    if(enc.result != DetectorResult::Invalid &&
       enc.encoding != Encoding::Invalid) return enc;

    nobom.readData(input);
    return nobom.Detect();
    */
}

















































