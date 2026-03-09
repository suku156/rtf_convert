// =====================================================
// Module : RtfFormatDetection (implementation)
// =====================================================
#include "RtfFormatDetection.h"
#include "Universal_Module/CommonEnum.h"
#include<fstream>
#include<vector>
#include<cstdint>
#include<string>
#include<sstream>
#include<ios>

// 介面給予繼承的函式的定義
std::vector<uint8_t> EncodingDetectorBase::readAllBytes(std::ifstream& input){
    input.clear();
    input.seekg(0, std::ios::beg);
    
    input.seekg(0, std::ios::end);
    std::streampos length = input.tellg();
    input.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(static_cast<size_t>(length));
    input.read(reinterpret_cast<char*>(buffer.data()), length);

    return buffer;
};

// DetectorBom 函式定義
void DetectorBom::readData(std::ifstream& input){
  //先重置狀態
  input.clear();
  input.seekg(0, std::ios::beg);
    
  bom_.assign(4, 0);
  input.read(reinterpret_cast<char*>(bom_.data()), 4);
  size_t actuallyRead = input.gcount();
  if (actuallyRead < 4) {// 未讀滿時，後面沒填到的地方保持 0
    std::fill(bom_.begin() + actuallyRead, bom_.end(), 0);
  }
    
  input.clear();
  input.seekg(0, std::ios::beg);
}
DetectOutput DetectorBom::Detect(){
    if (bom_[0] == 0xEF && bom_[1] == 0xBB && bom_[2] == 0xBF)
        return {DetectorResult::isOK , Encoding::UTF8_BOM};
    else if (bom_[0] == 0xFF && bom_[1] == 0xFE && bom_[2] == 0x00 && bom_[3] == 0x00)
        return {DetectorResult::isOK , Encoding::UTF32_LE};
    else if (bom_[0] == 0x00 && bom_[1] == 0x00 && bom_[2] == 0xFE && bom_[3] == 0xFF)
        return {DetectorResult::isOK , Encoding::UTF32_BE};
    else if (bom_[0] == 0xFF && bom_[1] == 0xFE)
        return {DetectorResult::isOK , Encoding::UTF16_LE};
    else if (bom_[0] == 0xFE && bom_[1] == 0xFF)
        return {DetectorResult::isOK , Encoding::UTF16_BE};
    
    return {DetectorResult::Invalid  , Encoding::Invalid};    
}

// DetectorAnsi 函式定義
void DetectorAnsi::readData(std::ifstream& input){
  // 預防指針被動過
  input.clear();
  input.seekg(0,std::ios::beg);
      
  std::stringstream buffer; // C++ 中用來方便處理字串的 多功能類別
  buffer << input.rdbuf(); // input 是一個 std::ifstream 的物件 使用.rdbuf()會取得其底層緩衝區,可一次性讀入所有檔案內容
  rtfContent_ = buffer.str(); // 將整份檔案已字串的型態存入
}
DetectOutput DetectorAnsi::Detect(){
    if(rtfContent_.empty()){
       return {DetectorResult::WrongStep , Encoding::Invalid};
    }
    //檢查編碼宣告是否不是ansi
    std::size_t pos;
    pos = rtfContent_.find("\\ansi");
    if(pos == std::string::npos){
      return {DetectorResult::Invalid , Encoding::Invalid};
    }

    //檢查是否有 utf 體系的控制碼 \uN 
    pos = rtfContent_.find("\\u");
    if(pos != std::string::npos){
    size_t end = pos + 2;
    if(rtfContent_[end] == '-' || std::isdigit(static_cast<unsigned char>(rtfContent_[end]))){
        return {DetectorResult::Invalid,Encoding::Invalid};
    }
    } 

    //檢查並確認使用的編碼
    std::string prefix = "\\ansicpg";
    pos = rtfContent_.find(prefix);
    if (pos != std::string::npos) {
    pos += prefix.size(); // 跳過關鍵字
    std::size_t end = pos;
    while (end < rtfContent_.size() && isdigit(static_cast<unsigned char>(rtfContent_[end]))) {
            end++;
    }
    std::string needNum = rtfContent_.substr(pos, end - pos);
    int ansNum = std::stoi(needNum);
    switch (ansNum)
    {
        case 950:return {DetectorResult::isOK,Encoding::Ansi_Big5}; // 繁中
        case 936:return {DetectorResult::isOK,Encoding::Ansi_GBK} ; // 簡中
        case 932:return {DetectorResult::isOK,Encoding::Ansi_JIS} ; // 日文
        case 949:return {DetectorResult::isOK,Encoding::Ansi_Korean} ;// 韓文
        case 1252:return {DetectorResult::isOK,Encoding::Ansi_Latin_1} ;// 西歐+英文  
        case 1250:return {DetectorResult::isOK,Encoding::Ansi_CEI} ;// 中歐語系
        case 1251:return {DetectorResult::isOK,Encoding::Ansi_Cyrillic} ;// 西里爾(烏,俄)
    }
    }

    return {DetectorResult::UnsupportedEncoding,Encoding::Invalid};
}

// DetectorNoBom 函式定義
void DetectorNoBom::readData(std::ifstream& input){
  bytes_ = readAllBytes(input); 
}
DetectOutput DetectorNoBom::Detect(){
  if (isValidUTF8(bytes_)) {
    return {DetectorResult::isOK,Encoding::UTF8_NoBOM};
  }
  return {DetectorResult::CannotDetect,Encoding::Invalid};
}
bool DetectorNoBom::isValidUTF8(const std::vector<uint8_t>& bytes){
size_t i = 0;
    while (i < bytes.size()) {
        uint8_t c = bytes[i];
        size_t remaining = 0;

        if (c <= 0x7F) {
            // 單一 ASCII (0xxxxxxx)
            i++;
            continue;
        }
        else if ((c & 0xE0) == 0xC0) {
            // 2-byte (110xxxxx 10xxxxxx)
            remaining = 1;
            if (c < 0xC2) return false; // 避免 overlong 編碼
        }
        else if ((c & 0xF0) == 0xE0) {
            // 3-byte (1110xxxx 10xxxxxx 10xxxxxx)
            remaining = 2;
        }
        else if ((c & 0xF8) == 0xF0) {
            // 4-byte (11110xxx 10xxxxxx 10xxxxxx 10xxxxxx)
            remaining = 3;
            if (c > 0xF4) return false; // Unicode 最大只到 U+10FFFF
        }
        else {
            return false; // 非法開頭
        }

        if (i + remaining >= bytes.size()) {
            return false; // 不夠 continuation byte
        }

        // 檢查 continuation bytes 是否都是 10xxxxxx
        for (size_t j = 1; j <= remaining; ++j) {
            if ((bytes[i + j] & 0xC0) != 0x80) {
                return false;
            }
        }

        i += remaining + 1; // 移到下一個字元
    }
    return true;
}

// 基本的rtf格式檢查函式定義
bool FileTypeDetector::isRTF(std::ifstream& input){
    input.clear();
    input.seekg(0,std::ios::beg);
    std::vector<unsigned char> buf;
    buf.assign(12,0);
    input.read(reinterpret_cast<char*>(buf.data()),buf.size());
    std::streamsize bytesRead = input.gcount();
    input.clear();
    input.seekg(0,std::ios::beg);

    if(bytesRead < 5) return false;

    if(bytesRead >= 5 && // ansi 編碼或 無bom utf8
       buf[0] == 0x7B && buf[1] == 0x5C && buf[2] == 0x72 &&
       buf[3] == 0x74 && buf[4] == 0x66)
    return true;
        
    if(bytesRead >= 8 && // UTF-8 + BOM
                buf[0] == 0xEF && buf[1] == 0xBB && buf[2] == 0xBF &&
                buf[3] == 0x7B && buf[4] == 0x5C && buf[5] == 0x72 &&
                buf[6] == 0x74 && buf[7] == 0x66)
    return true;

    if(bytesRead >= 12 && // UTF-16 LE
                buf[0] == 0xFF && buf[1] == 0xFE &&
                buf[2] == 0x7B && buf[4] == 0x5C && buf[6] == 0x72 &&
                buf[8] == 0x74 && buf[10] == 0x66)
    return true;

    if(bytesRead >= 12 &&
                buf[0] == 0xFE && buf[1] == 0xFF &&
                buf[3] == 0x7B && buf[5] == 0x5C && buf[7] == 0x72 &&
                buf[9] == 0x74 && buf[11] == 0x66)
    return true;

    return false;
}
bool FileTypeDetector::isFileTooShort(std::ifstream& input,std::size_t minSize){
  input.clear();
    
  std::streampos oldPos = input.tellg();
  input.seekg(0,std::ios::end);
  std::streamsize size = input.tellg();

  //把指針放回原本的位置
  input.seekg(oldPos);

  return size < static_cast<std::streamsize>(minSize);
}

// 類別 : Facade 模式,函式定義
DetectOutput EncodingDetector::detectEncoding(std::ifstream& input){
    // 檢查是否是 rtf 格式
    if(!FileTypeDetector().isRTF(input)){
      return {DetectorResult::NotRTF,Encoding::Invalid};
    }

    // 檢查檔案是否過小
    if(FileTypeDetector().isFileTooShort(input,5)){
      return {DetectorResult::FileTooSmall,Encoding::Invalid};
    }

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
  
}

















































