// =====================================================
// Module : OutputPathResolver (implementation)
// =====================================================
#include "OutputPathResolver.h"
#include <filesystem>
#include <optional>
#include <system_error>

namespace OPResolver{
  
  ResolverResult OutputPathResolver::resolve(const ResolverRequest& request) const{
    ResolverResult result;
    
    // 拿到目標檔案的中間路徑
    result.relativeSubDir = buildRelativeSubDir(request);
    
    // 組裝路徑 用輸出資料夾 加上 中間路徑
    result.parentDir = request.baseOutputDir / result.relativeSubDir;
    
    // 取得目標的檔名(第一版先固定成 txt 檔)
    std::filesystem::path outputFilename = request.inputFile.filename();
    outputFilename.replace_extension(L".txt");

    // 最終的結果 完整路徑含檔名
    result.finalPath = result.parentDir / outputFilename;

    return result;
  }
  

  std::filesystem::path OutputPathResolver::buildRelativeSubDir(const ResolverRequest& request) const{
    if((!request.taskRootDir.has_value()) || 
        request.preserveRelativeStructure == false ||
        (request.mode != PathResolveMode::DirectoryRecursive))
    {
      return {}; 
    }
    
    const std::filesystem::path& root = request.taskRootDir.value();
    const std::filesystem::path& parent = request.inputFile.parent_path();

    std::error_code ec;
    std::filesystem::path rel = std::filesystem::relative(parent,root,ec);

    if(ec) return {};
    if(rel == "." || rel.empty()) return {};

    return rel;
  }
}