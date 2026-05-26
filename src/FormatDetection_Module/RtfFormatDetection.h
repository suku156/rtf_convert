// =====================================================
// Module  : RtfFormatDetection
// Author  : suku156
// Purpose : 用來偵測目標 rtf 的格式
// Layer   : formatdetect
//
// Depend  :
//   CommonEnum 
// Used by :
//   RTFProcessor   
//
// Notes :
//   用來在目標rtf檔上找到並儲存處裡需要的資訊
//   基礎編碼等等,會做一些非常基礎的驗證(是不是rtf檔案等等)
// =====================================================
#pragma once
#include "Universal_Module/CommonEnum.h"
#include<fstream>
#include<vector>
#include<cstdint>
#include<string>
#include<optional>

// 可能有的 Bom 
enum class BomType {
    None,
    Utf8,
    Utf16LE,
    Utf16BE,
    Utf32LE,
    Utf32BE
};

// 用於承裝檔案是否過小用得資訊容器
struct SizeProbeInfo {
  // 實際找到的
  std::size_t fileSize = 0;
  // 設定的允許最小值
  std::size_t minRequiredSize = 0;
  bool isTooShort = false;
};

// 新的容器用於承裝偵測報告
struct DetectOutputNew{
  // 可能的BOM 型態
  BomType bomtype = BomType::None;
  // 偵測 檔案 byte 是否過短的結果 
  SizeProbeInfo sizeInfo;
  // 偵測是否符合開頭 { 後除了空格與換行外必須是 \rtfX(X是數字) 的規則
  bool hasValidRtfRootHeader = false;
  std::optional<int> ansiCodePage;
};

// 類別 : Facade 模式 , 使用此模式單獨將偵測類別包成一種可簡易使用的類別
class EncodingDetector{
public:
  DetectOutputNew detectEncoding(std::ifstream& input);
private:
  BomType detectBom(std::ifstream& input);
  SizeProbeInfo isFileTooShort(std::ifstream& input,std::size_t minSize);
  bool rtfSymbolCheck(std::string_view strView);
  std::optional<int> ansiCodePageCheck(std::string_view strView);
};