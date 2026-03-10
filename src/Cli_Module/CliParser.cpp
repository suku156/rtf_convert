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
      else if(arg == L"--format" ){
        if(i + 1 >= argc){
          result.message = L"--format 後面缺少格式名稱";
          return result;
        }

        std::wstring fmt = argv[++i];
        fmt = toLower(fmt);
        if(fmt == L"txt"){
          result.config.format = OutputFormat::Txt; 
        }
        else if(fmt == L"md"){
          result.config.format = OutputFormat::Md;
        }
        else if(fmt == L"html"){
          result.config.format = OutputFormat::Html;  
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

  std::wstring usage(){
    return 
    L"\n\n"
    L"Usage:\n"
    L"  rtfconvert --input <file|dir> --output <dir> --format <txt|md|html>\n"
    L"\n"
    L"Options:\n"
    L"  --input     指定輸入檔案或資料夾\n"
    L"  --output    指定輸出資料夾(不輸入使用預設)\n"
    L"  --format    輸出格式 (txt | md | html)(不輸入使用預設)\n"
    L"\n"
    L"Example:\n"
    L"  rtfconvert --input test.rtf --output out --format txt\n";
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
    std::wcout << L"  --help      顯示此說明\n";
  }

  void printVersion(){
    std::wcout << AppInfo::Name
               << L" "
               << AppInfo::Version
               << L"\n";
  }
}

