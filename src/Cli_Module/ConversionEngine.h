// =====================================================
// Module  : CliParser
// Author  : suku156
// Purpose : 依據終端機輸入之資料決定如何呼叫
// Layer   : Cli
//
// Depend  :
//   CliParser
//   RTFProcessor
//   MyThread  
// Used by :
//   rtfconvert   
//
// Notes :
//   依據終端機給定的資料進行判斷如何呼叫主流程
// =====================================================

#pragma once

// forward declaration
namespace Cli{
  struct  ParseResult; 
}

namespace App{
  enum class AppExitCode{
    Success,
    Fail,
    CliError,
    RunTimeError
  };

  class ConversionEngine{
  public:
  AppExitCode run(const Cli::ParseResult& pr);
  private:
  }; 
}
