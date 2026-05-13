// =====================================================
// Module  : pictProcessor
// Author  : suku156
// Purpose : 用來處理 rtf 內部得圖片相關 
// Layer   : pictProcessors
//
// Depend  :
//   ErrorSystem 
//   ErrorHandle
//   OutputDirGuard
//   LogSystem
//   Console
//   
// Used by :
//   RTFProcessor   
//
// Notes :
//   負責處理圖片區塊,將其轉換為對應格式檔案
//   目前支援 :　ｐｎｇ　ｊｐｇ　ｗｍｆ　ｅｍｆ
//   處理完的圖片區塊會替換為圖片標記符(分為正常與錯誤標記符)
// =====================================================
#pragma once
#include<string>
#include<cstddef>
#include<cstdint>
#include<vector>
#include<filesystem>
#include<optional>
#include<string_view>

// forward declaration
class ErrorHandle;
class OutputDirGuard;
class logSystem;

//用來記錄位置用的類別
struct ReplaceTask{
  std::size_t start;
  std::size_t end;
  std::string text;  
};

enum class PictFormat {WMF, EMF, PNG,JPEG,UNKNOWN};

//將 byte 轉化為圖片所需的最小結構
struct PictInfo{
  PictFormat format = PictFormat::UNKNOWN;
  std::vector<uint8_t> bytes;
  std::filesystem::path outputPath; // 將由 總控PictDisassembler 決定
};

class ProcessRunner{
public:
   static int run(const std::filesystem::path& exe,
                  const std::vector<std::filesystem::path>& args);
private:
   static std::wstring quoterArgW(const std::wstring& s);
   static std::string  quoteArgA(const std::string& s);
};

class imageProcessor{
public:
  bool saveRaw(const PictInfo& info) const;
  bool convertToPng(const std::filesystem::path& inWmfOrEmf,
                    const std::filesystem::path& outPng) const;
  bool process(const PictInfo& info, bool wantPng = false) const;
private:
  static std::string quotePath(const std::filesystem::path& p);
};

struct PictHeaderInfo{
  PictFormat format = PictFormat::UNKNOWN;
  std::optional<int> picw;
  std::optional<int> pich;
  std::optional<int> picwgoal;
  std::optional<int> pichgoal;
};

// 圖片處理類別的回傳值,讓主流程可以依據其回傳進行應對
enum class PictProcessResult{
  OK,         
  SkipPict,   
  AbortFile,  
  AbortGlobal 
};

// 圖片掃描時的回傳類別
struct GroupScanReport{
  size_t groupEnd = std::string::npos;
  bool braceBalanced = true; 
  bool pictHexInterrupted = false; 
  bool detectedNewGroupStart = false; 
  bool HexBroken = false; 
};

// 圖片區塊的總統籌類別
class PictDisassembler{
  ErrorHandle& errorHandler_;
  OutputDirGuard& outputDir_;
  imageProcessor imgProcessor_;
  size_t pictCount_ = 0;
public:
  PictDisassembler(ErrorHandle& errorhandler,OutputDirGuard& outputDir)
   : errorHandler_(errorhandler) , outputDir_(outputDir){}
  PictProcessResult process(std::string& rtfContent,logSystem& logger); 
private:
  GroupScanReport scanGroup(const std::string& text,size_t groupStart);
  size_t findPictHexStart(std::string_view text,size_t start,logSystem& logger) const;
  bool isWmfHexTooShort(std::string_view hex);
  PictHeaderInfo parsePictHeader(std::string_view header);
  PictFormat detectPictFormat(std::string_view header);
  std::optional<int> readControlInt(std::string_view header, std::string_view key);
  static inline int hexValue(unsigned char c);
  std::vector<uint8_t> extractHexBytes(std::string_view hexView);
  std::optional<std::filesystem::path> makeOutputPath(PictFormat fmt,size_t index);
  bool isInsideNonShpPictGroup(std::string_view content, size_t pictPos);
  void treatNonshppictGroup(std::vector<ReplaceTask>& tasks,std::string_view rtfview);
};


