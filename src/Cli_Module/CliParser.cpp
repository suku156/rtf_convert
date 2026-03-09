// =====================================================
// Module : CliParser (implementation)
// =====================================================
#include "CliParser.h"
#include "Universal_Module/CommonEnum.h"
#include<string>

namespace Cli{
  ParseResult parse(int argc,wchar_t* argv[]){
    ParseResult result;
    bool hasInput = false;
    bool hasOutput = false;
    bool hasFormat = false;
    
    for(int i = 1; i < argc; i++){
       std::wstring arg = argv[i];
       
       if(arg == L"--input" && i+1 < argc){
        result.config.inputPath = argv[++i];
        hasInput = true;
       }
       else if(arg == L"--output" && i+1 < argc){
        result.config.outputDir = argv[++i];
        hasOutput = true;
       }
       else if(arg == L"--format" && i+1 < argc){
        std::wstring fmt = argv[++i];
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
          result.ok = false;
          result.message = L"未知的 format";
          return result; 
        }
        hasFormat = true;
       }
       else{
        result.ok = false;
        result.message = L"未知參數或缺少值";
        return result;  
       }
    }

    if(!hasInput || !hasOutput || !hasFormat){
      result.ok = false;
      result.message = L"缺少必要參數";
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
    L"  --output    指定輸出資料夾\n"
    L"  --format    輸出格式 (txt | md | html)\n"
    L"\n"
    L"Example:\n"
    L"  rtfconvert --input test.rtf --output out --format txt\n";
  }
}

