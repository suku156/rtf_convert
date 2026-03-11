#include "OutputDirGuard.h"
#include "Universal_Module/Console.h"
#include "Universal_Module/CommonEnum.h"
#include<filesystem>
#include<system_error>

bool OutputDirGuard::isReady() const{
  return hasDir_;
}
// 安全使用方法 : 在使用 ensure() 函式並回傳 Success 後 可確保存在輸出資料夾
const std::filesystem::path& OutputDirGuard::path() const{
  return outputDir_;
}
EnsureDirResult OutputDirGuard::ensure(){
    if(hasDir_) return EnsureDirResult::Success;
    std::error_code ec;
  
    //1.如果已經有同名檔案依據狀況執行
    if(std::filesystem::exists(outputDir_,ec)){
      if(ec){
        return EnsureDirResult::VerifyFailed;
      }
      
      // 目標不是資料夾
      if(!std::filesystem::is_directory(outputDir_,ec)){ 
        return EnsureDirResult::NotDirectory;
      }
      
      // 已經存在資料夾就依據指令決定動作
      if(policy_ == Common::ExistingDirPolicy::Reject){
        return EnsureDirResult::AlreadyExists;
      }
      else if(policy_ == Common::ExistingDirPolicy::Overwrite){
        // 因為是每個檔案自己一個資料夾所以可用,如果用共同資料夾要小心
        std::filesystem::remove_all(outputDir_,ec);
        if(ec) return EnsureDirResult::RemoveFailed;
      }
    }
    
    //2.沒有同名檔案就嘗試建立資料夾
    std::filesystem::create_directories(outputDir_,ec);
    if(ec){  // 建立失敗回傳錯誤
      return EnsureDirResult::CreateFailed;
    };

    //二次確認
    if(!std::filesystem::is_directory(outputDir_,ec) || ec){
      return EnsureDirResult::VerifyFailed;
    }
    
    hasDir_ = true;
    return EnsureDirResult::Success;
}
DirCheckResult OutputDirGuard::checkDirectory(const std::filesystem::path& p){
  std::error_code ec;

  if(!std::filesystem::exists(p, ec)){
    if(ec) return DirCheckResult::StatusFailed;
    return DirCheckResult::NotExist;
  }

  if(!std::filesystem::is_directory(p, ec)){
    if(ec) return DirCheckResult::StatusFailed;
    return DirCheckResult::NotDirectory;
  }

  return DirCheckResult::Ok;
}