#include "Cli_Module/ConversionEngine.h"
#include "Cli_Module/CliParser.h"
#include "Universal_Module/OutputDirGuard.h"
#include "MainProcess_Module/RTFProcessor.h"
#include "thread_Moudle/MyThread.h"
#include "Universal_Module/Console.h"
#include <system_error>
#include <filesystem>
#include <iostream>

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

namespace App{
  AppExitCode ConversionEngine::run(const Cli::ParseResult& pr){
    // 確認是否真的是可以用的input
    {
     std::error_code ec;
     auto st = std::filesystem::status(pr.config.inputPath,ec);
     if (ec) {
      std::wcout << L"1.輸入檔案不符合規定,程式未執行\n";
      return AppExitCode::CliError;
     }
     if (!std::filesystem::exists(st)) {
      std::wcout << L"1.輸入檔案不符合規定,程式未執行\n";
      return AppExitCode::CliError;
     }
    }
    
    // 確保 outputDir 存在 使用原有模組達成
    OutputDirGuard fileOut(pr.config.outputDir);
    if(!fileOut.ensure()){ // ensure 後沒有回傳 false 就可以確保有輸出資料夾
      std::wcout << L"輸出資料夾不符合規定,程式未執行\n";
      return AppExitCode::RunTimeError;
    }
    
    // 在指定資料夾中建立輸出資料夾
    // 擷取不含副檔名的檔案名稱
    std::wstring baseName = pr.config.inputPath.stem().wstring();
    // 檢查與替換檔名中會造成問題的空格
    baseName = sanitizeFileName(baseName);

    // 在目標資料夾內建立一個輸出資料夾
    std::filesystem::path outputDir = fileOut.path() / baseName;
    OutputDirGuard useOutDir(outputDir);
    if(!useOutDir.ensure()){
      std::wcout << L"目標資料夾內建立輸出資料夾失敗,程式未執行\n";
      return AppExitCode::RunTimeError;
    }

    std::error_code ec;
    //目標如果是單獨檔案的話
    if (std::filesystem::is_regular_file(pr.config.inputPath, ec)) {
      // 單檔：直接呼叫 processor
      RTFProcessor rtfprocessor;
      bool flag = rtfprocessor.processFile(pr.config.inputPath,
                                           useOutDir.path(),
                                           pr.config.format,
                                           ProcessMode::SingleFile);
      if(flag){
        return AppExitCode::Success;  
      }else{
        return AppExitCode::Fail;
      }
    }
    else if (std::filesystem::is_directory(pr.config.inputPath, ec)) {
      RTFDirectoryRunner Drunner;
      ProgressObserver ProOB;
      Drunner.run(pr.config.inputPath,ProOB,pr);  
    }

    return AppExitCode::Fail;
  }
}

