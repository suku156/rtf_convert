// =====================================================
// Module : OutputPathResolver (implementation)
// =====================================================
#include "OutputPathResolver.h"
#include "Universal_Module/Console.h"

#include <filesystem>
#include <optional>
#include <system_error>
#include <string>
#include <mutex>
#include <mutex>
#include <unordered_set>

namespace{
  // 用來檢查與替換檔名中有影響的空格
  std::wstring sanitizeFileName(const std::wstring& basename){
    std::wstring result;
    result.reserve(basename.size());

    for(wchar_t c:basename){
    //如果有找到全形或特別的空白
    if(c == L'\u3000' || c == L'\u00A0' || c == L'\u2002' || c == L'\u2003' ||
        c == L'\u2009' || c == L'\u202F' || c == L'\u205F' || c == L'\u200B')
    {
        result.push_back(L' ');
    }else if(c == L' '){
        result.push_back(L' ');
    }else if(c == L'<' || c == L'>' || c == L':' || c == L'"' ||
                c == L'/' || c == L'\\' || c == L'|' || c == L'?' || c == L'*') // 防止非法檔案名稱
    {
        result.push_back(L'_');
    }else{
        result.push_back(c);
    }
    }

    // 去除開頭或結尾空白（避免 Windows 檔名非法）
    while (!result.empty() && std::iswspace(result.front())) result.erase(result.begin());
    while (!result.empty() && std::iswspace(result.back()))  result.pop_back();

    //如果有對檔案名稱進行改動,傳入資料流
    if(result != basename){
    Console::ensureWcerr(L"[Notice] 檔名中包含不安全字元，已自動修正。\n");
    }

    return result;
  } 
}

namespace OPResolver{
  
  ResolverResult OutputPathResolver::resolve(const ResolverRequest& request) const{
    ResolverResult result;
    
    // 拿到目標檔案的中間路徑
    result.relativeSubDir = buildRelativeSubDir(request);
    
    // 組裝路徑 用輸出資料夾 加上 中間路徑
    std::filesystem::path baseDir = request.baseOutputDir / result.relativeSubDir;
    
    // 取得目標的檔名並進行基礎的檢查
    std::filesystem::path basename = request.inputFile.stem();
    std::wstring safeName = sanitizeFileName(basename.stem());
    if(safeName.empty() || safeName == L"." || safeName == L".."){
      safeName = L"output";
    }
    std::filesystem::path safeBaseName = safeName;
    
    // 將得到的路徑與檔名組合成 最終輸出檔案所在的資料夾路徑
    result.parentDir = baseDir / safeBaseName;
    
    // 依據格式決定副檔名
    std::wstring ext;
    switch(request.format){
      case OutputFormat::txt : ext = L".txt" ; break;
      case OutputFormat::md  : ext = L".md"  ; break;
      case OutputFormat::html: ext = L".html"; break;
      default : ext = L".txt"; break;
    }
    
    // 建立最終檔名(含有副檔名)
    std::filesystem::path outputFilename = safeBaseName;
    outputFilename += ext;

    // 最終完整檔案路徑
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

  std::optional<std::filesystem::path> 
  OutputPathRegistry::reserveUniqueDir(
  const std::filesystem::path& parent,
  const std::wstring& baseName,
  CollisionPolicy policy)
  {
    std::lock_guard<std::mutex> lockguard(mutex_);
     
    //先組合路徑進行檢查
    std::filesystem::path candidate = parent / baseName;
    std::wstring key = candidate.wstring();
    
    //名單放在記憶體比較快先檢查名單
    if(reserved_.count(key) == 0){
      std::error_code ec;
      bool existsOnDisk = std::filesystem::exists(candidate, ec);
    
      if(ec){
        return std::nullopt; 
      }
      if(!existsOnDisk){ // 代表磁碟檢查也通過
        reserved_.insert(key); // 登入名單中
        return candidate;
      }else{
        switch(policy){
          case CollisionPolicy::ErrorIfExists :
          return std::nullopt;
          break;
          case CollisionPolicy::Overwrite :
          break;
        }
      }

      
    }
      
    // 會走到此處代表需要嘗試在檔名後方加上數字
    
    return std::nullopt;// 暫時放這個
  } 
}