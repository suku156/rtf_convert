// =====================================================
// Module : ConversionTaskBuilder (implementation)
// =====================================================

#include "ConversionTaskBuilder.h"
#include "ConversionTask.h"
#include "NormalizedConversionRequest.h"
#include "Universal_Module/CommonEnum.h"
#include <filesystem>
#include <optional>
#include <cstddef>
#include <thread>
#include <iostream>
#include <system_error>


namespace taskBuilder{
   BuildResult ConversionTaskBuilder::build(const NormalizedConversionRequest& request){
      BuildResult result;
      
      if(request.inputPath.empty()){
        result.ok = false;
        result.message = L"inputPath is empty(輸入路徑為空)";
        return result;
      }
      // 保底做一次輸入路徑絕對路徑化
       std::filesystem::path input  = std::filesystem::absolute(request.inputPath);

      // 檢查有無給定指定 outputDir　路徑
      std::error_code ec;
      if(request.outputDir.has_value()){
        result.task.outputDir = *request.outputDir;
      }else{
          if (std::filesystem::is_regular_file(input, ec)){
            result.task.outputDir = input.parent_path(); 
          }
          else if(std::filesystem::is_directory(input, ec)){
            result.task.outputDir = input;  
          }
          else{
            result.task.outputDir = input.parent_path();  
          }
      }
      
      // output 補做正規化
      result.task.outputDir = std::filesystem::absolute(result.task.outputDir);
      result.task.inputPath = input;
      
      // 檢查並補預設 format
      if(!request.format){
        result.task.format = Common::OutputFormat::Txt;
      }else{
        result.task.format = *request.format;
      }
      
      // 直接傳遞的資料
      result.task.dirPolicy = request.dirPolicy;
      result.task.recursive = request.recursive;

      // 沒給指定狀態就用預設行為來判斷的資料
      if(request.preserveRelativeStructure.has_value()){
        result.task.preserveRelativeStructure = request.preserveRelativeStructure.value();
      }else{
        result.task.preserveRelativeStructure = 
        (result.task.recursive && result.task.inputPath == result.task.outputDir);
      }
      
      result.task.threadCount = normalizeThreadCount(request.threadCount);
      
      result.ok = true;
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