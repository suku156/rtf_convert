#include "OutputDirGuard.h"
#include "Universal_Module/Console.h"
#include<filesystem>
#include<system_error>

bool OutputDirGuard::isReady() const{
  return hasDir_;
}
// 安全使用方法 : 在使用 ensure() 函式並回傳 true 後 可確保存在輸出資料夾
const std::filesystem::path& OutputDirGuard::path() const{
  return outputDir_;
}
bool OutputDirGuard::ensure(){
    if(hasDir_) return true;
    std::error_code ec;
  
    //1.如果已經有檔案,先檢查原本路徑使否真的有指向物件
    if(std::filesystem::exists(outputDir_,ec)){
      if(!ec && std::filesystem::is_directory(outputDir_,ec)){
        hasDir_ = true;
        return true; // 如果該物件是資料夾就 OK
      }else{
        return false; // 不是就是檔案 回傳 否
      }
    }
    
    //2.嘗試建立資料夾
    std::filesystem::create_directories(outputDir_,ec);
    if(ec){  // 真的建立失敗在回傳 錯誤
      Console::ensureWcerr(L"[create_directories failed]\npath = " + outputDir_.wstring());
      return false;
    };

    //3.二次確認
    if(!std::filesystem::is_directory(outputDir_)){
      return false;
    }
    
    hasDir_ = true;
    return true;
    
}