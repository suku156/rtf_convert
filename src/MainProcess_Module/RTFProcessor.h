// =====================================================
// Module  : RTFProcessor
// Author  : suku156
// Purpose : 主要執行任務的模組(主流程)
// Layer   : Main process
//
// Depend  :
//   Universal_Module(ALL)
//   LogSystem
//   ErrorHandle
//   ErrorSystem
//   RtfFormatDetection
//   pictProcessor
//   ErrorDetector
//   Decode
//   GroupProcessor
//   textProcessor
//   Document
//   Renderer
// 
// Used by :
//   ConversionEngine
//   MyThread
//   
// Notes :
//   主要使用的流程模組
//   IProgressObserver* observer_ = nullptr 
//   如果維持預設的空指標狀態就會是安靜模式
//   也就是建構時不給東西
//   observer == nullptr 時，RTFProcessor 以安靜模式運作；
//   適合批次/多執行緒情境，由外層統一負責進度輸出。
// =====================================================
#pragma once
#include "Universal_Module/CommonEnum.h"  // 多個模組共用的 enum
#include<filesystem>
#include<optional>


struct FileProcessRequest;
class IProgressObserver;
struct ProgressEvent;

// 主流程
class RTFProcessor{
  IProgressObserver* observer_ = nullptr;
public:
  explicit RTFProcessor(IProgressObserver* observer = nullptr) : observer_(observer) {}
  bool processFile(const FileProcessRequest& req);
private:
  void notify(const ProgressEvent& event);
};


