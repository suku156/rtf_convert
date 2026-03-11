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
//    
// =====================================================
#pragma once
#include "Universal_Module/CommonEnum.h"  // 多個模組共用的 enum
#include<filesystem>
#include<optional>

struct FileProcessRequest;

// 主流程
class RTFProcessor{
public:
  bool processFile(const FileProcessRequest& req);
};


