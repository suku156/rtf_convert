// =====================================================
// Module : ConversionTaskBuilder (implementation)
// =====================================================
#pragma once
#include "ConversionTaskBuilder.h"
#include "ConversionTask.h"
#include "NormalizedConversionRequest.h"
#include "Universal_Module/CommonEnum.h"
#include <filesystem>
#include <optional>
#include <cstddef>
#include <thread>
#include <iostream>


namespace taskBuilder{
   BuildResult ConversionTaskBuilder::build(NormalizedConversionRequest request){
      BuildResult result;
      
      if(request.inputPath.empty()){
        result.ok = false;
        result.message = L"inputPath is empty(輸入路徑為空)";
        return result;
      }
      if(request.outputDir.empty()){
        result.ok = false;
        result.message = L"outputDir is empty(輸出資料夾路徑為空)";
        return result;
      }

      result.task.inputPath = request.inputPath;
      result.task.outputDir = request.outputDir;
      
      // 直接傳遞的資料
      result.task.format = request.format;
      result.task.dirPolicy = request.dirPolicy;
      result.task.recursive = request.recursive;

      // 沒給指定狀態就用預設行為來判斷的資料
      if(request.preserveRelativeStructure.has_value()){
        result.task.preserveRelativeStructure = request.preserveRelativeStructure.value();
      }else{
        if(result.task.inputPath == result.task.outputDir){
            result.task.preserveRelativeStructure = true;
        }else{
            result.task.preserveRelativeStructure = false;
        }
      }
      
      result.task.threadCount = normalizeThreadCount(request.threadCount);
      
      return result;
    }

    size_t ConversionTaskBuilder::normalizeThreadCount(std::optional<size_t> requested){
      constexpr size_t kMaxThreads = 16;
      size_t n = requested.value_or(std::thread::hardware_concurrency());

      if(n == 0){
        n = 4;
      }

      n = std::min(n,kMaxThreads);
      return n;
    }

    
}