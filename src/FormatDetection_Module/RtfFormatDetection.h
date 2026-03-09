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

// 用來存放結果的小容器
struct DetectOutput{
  DetectorResult result = DetectorResult::Invalid;
  Encoding encoding = Encoding::Invalid;
};

//偵測用的基底類別(介面)
class EncodingDetectorBase{
public:
  virtual ~EncodingDetectorBase() = default;
  virtual DetectOutput Detect() = 0;
  virtual void readData(std::ifstream& input) = 0;
protected:
  static std::vector<uint8_t> readAllBytes(std::ifstream& input);
};

//類別:用來依據Bom 來判斷是否為 UTF-8 or UTF-16
class DetectorBom : public EncodingDetectorBase{
  std::vector<uint8_t> bom_;
public:
  void readData(std::ifstream& input) override;
  DetectOutput Detect()override;
};

//類別:用來偵測ANSI
class DetectorAnsi : public EncodingDetectorBase{
  std::string rtfContent_;
public:
  void readData(std::ifstream& input) override;
  DetectOutput Detect() override;
};

//類別:用來檢測是否為 no-Bom 的 UTF-8
class DetectorNoBom : public EncodingDetectorBase{
  std::vector<uint8_t> bytes_;
public:
  void readData(std::ifstream& input) override;
  DetectOutput Detect() override;
private:
  bool isValidUTF8(const std::vector<uint8_t>& bytes);
};

// 一些基本rtf 格式的檢查
class FileTypeDetector{
public:
  bool isRTF(std::ifstream& input);
  bool isFileTooShort(std::ifstream& input,std::size_t minSize);
};

// 類別 : Facade 模式 , 使用此模式單獨將偵測類別包成一種可簡易使用的類別
class EncodingDetector{
public:
  DetectOutput detectEncoding(std::ifstream& input);
};