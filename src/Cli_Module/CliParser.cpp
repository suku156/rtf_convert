// =====================================================
// Module : CliParser (implementation)
// =====================================================
#include "CliParser.h"
#include "Universal_Module/CommonEnum.h"
#include "Universal_Module/Version.h"
#include<string>
#include<cwctype>
#include<algorithm>
#include<iostream>
#include<exception>
#include<system_error>

namespace{
  std::wstring toLower(std::wstring s){
    for(auto& c : s){
      c = std::towlower(c);
    }
    return s;
  }
}

namespace Cli{
  ParseResult parse(int argc,wchar_t* argv[]){
    ParseResult result;
    bool hasInput = false;
    bool hasDirPolicy = false;
   
    for(int i = 1; i < argc; i++){
      std::wstring arg = argv[i];
      
      if(arg == L"--input" ){
        if(i + 1 >= argc){
          result.message = L"--input 後面缺少檔案路徑";
          return result;
        }
        result.config.inputPath = argv[++i];
        hasInput = true;
      }
      else if(arg == L"--output"){
        if(i + 1 >= argc){
          result.message = L"--output 後面缺少輸出資料夾路徑";
          return result;
        }
        result.config.outputDir = argv[++i];
      }
      else if(arg == L"--help"){
        result.showhelp = true;
        return result;
      }
      else if(arg == L"--version"){
        result.shoeversion = true;
        return result;
      }
      else if(arg == L"--overwrite"){
        if(hasDirPolicy){
          result.ok = false;
          result.message = L"輸出資料夾處理策略只能指定一種，不能同時使用多個選項";
          return result;
        }
        result.config.dirPolicy = Common::ExistingDirPolicy::Overwrite;
        hasDirPolicy = true;
      }
      else if(arg == L"--renamewithsuffix"){
        if(hasDirPolicy){
          result.ok = false;
          result.message = L"輸出資料夾處理策略只能指定一種，不能同時使用多個選項";
          return result;
        }
        result.config.dirPolicy = Common::ExistingDirPolicy::RenameWithSuffix;
        hasDirPolicy = true;
      }
      else if(arg == L"--reject"){
        if(hasDirPolicy){
          result.ok = false;
          result.message = L"輸出資料夾處理策略只能指定一種，不能同時使用多個選項";
          return result;
        }
        result.config.dirPolicy = Common::ExistingDirPolicy::Reject;
        hasDirPolicy = true;
      }
      else if(arg == L"--recursive"){
        result.recursive = true;
      }
      else if(arg == L"--preserve-structure"){
        if(result.config.preserveRelativeStructure.has_value()){
          result.ok = false;
          result.message = L"資料夾遞迴輸出結構選項不能同時指定多個。";
          return result;
        }
        result.config.preserveRelativeStructure = true;
      }
      else if(arg == L"--flat-output"){
        if(result.config.preserveRelativeStructure.has_value()){
          result.ok = false;
          result.message = L"資料夾遞迴輸出結構選項不能同時指定多個。";
          return result;
        }
        result.config.preserveRelativeStructure = false;
      }
      else if(arg == L"--thread"){
        if(i+1 >= argc){
          result.ok = false;
          result.message = L" --thread 需指定執行緒數量。";
          return result;
        }
        
        try{
          std::wstring threadstring = argv[++i];
          size_t pos = 0;
          unsigned long long raw = std::stoull(threadstring,&pos);

          if(pos != threadstring.size()){
            result.ok = false;
            result.message = L"--thread 的值必須是正整數";
            return result;
          }
          
          size_t threadresult = static_cast<size_t>(raw);
          
          if(threadresult == 0){
            result.ok = false;
            result.message = L"多執行緒數量不可設置為 0";
            return result;
          }

          if(threadresult > 16){
            result.ok = false;
            result.message = L"多執行緒數量不可超過 16。";
            return result;
          }
          result.config.threadCount = threadresult;
        }
        catch(const std::exception&){
          result.ok = false;
          result.message = L"--thread 的值必須是正整數";
          return result;
        }
      }
      else if(arg == L"--format" ){
        if(i + 1 >= argc){
          result.message = L"--format 後面缺少格式名稱";
          return result;
        }

        std::wstring fmt = argv[++i];
        fmt = toLower(fmt);
        if(fmt == L"txt"){
          result.config.format = Common::OutputFormat::Txt; 
        }
        else if(fmt == L"md"){
          result.config.format = Common::OutputFormat::Md;
        }
        else if(fmt == L"html"){
          result.config.format = Common::OutputFormat::Html;  
        }
        else{
          result.message = L"未知的 format: " + fmt;
          return result; 
        }
      }
      else{
        result.message = L"未知參數: " + arg;
        return result;
      }
    }
    
    if(!hasInput){
      result.ok = false;
      result.message = L"缺少必要參數 --input";
      return result;
    }
    result.ok = true;
    return result;
  }

  void printHelp(){
    std::wcout << L"Usage:\n";
    std::wcout << L"  rtfconvert --input <path> [--output <dir>] [--format txt]\n\n";
    std::wcout << L"Options:\n";
    std::wcout << L"  --input     指定輸入檔案或資料夾\n";
    std::wcout << L"  --output    指定輸出資料夾 \n";
    std::wcout << L"(--output 不輸入的話.目標為檔案時預設為檔案所在資料夾,目標為資料夾的話預設為目標資料夾)\n";
    std::wcout << L"  --format    輸出格式\n";
    std::wcout << L"(--format 目前支援 : txt|md|html 三種格式)\n";
    std::wcout << L"  --version 顯示目前的版本號\n";
    std::wcout << L"  --overwrite 碰到同名資料夾會採取覆蓋策略,不能與安全模式及新增檔案模式並存\n";
    std::wcout << L"  --renamewithsuffix 碰到同名資料夾會採取新增檔案策略,不能與安全模式及覆蓋檔案模式並存\n";
    std::wcout << L"  --reject (為預設處理模式)碰到同名資料夾會採取安全策略,不能與新增檔案模式及覆蓋檔案模式並存\n";
    std::wcout << L"  --recursive 目標為資料夾的話會遞迴處理包含的資料夾\n";
    std::wcout << L"  --preserve-structure 設定資料夾遞迴模式下保留所有子資料夾中間路徑\n";
    std::wcout << L"  --preserve-structure 補充:(不設定會用預設行為)(不能與不保留中間路徑並存)(非資料夾遞迴模式會無視此指令)\n";
    std::wcout << L"  --flat-output 設定資料夾遞迴模式下'不'保留所有子資料夾中間路徑\n";
    std::wcout << L"  --flat-output 補充:(不設定會用預設行為)(不能與保留中間路徑並存)(非資料夾遞迴模式會無視此指令)\n";
    std::wcout << L"  --thread 用於決定遞迴模式有多少執行緒(非遞迴模式會無視)(設定數值需為正整數 1~16)\n";
    std::wcout << L"  --help      顯示此說明\n";
  }

  void printVersion(){
    std::wcout << AppInfo::Name
               << L" "
               << AppInfo::Version
               << L"\n";
  }
}

