// =====================================================
// Module  : Executor
// Author  : suku156
// Purpose : 依據資料建構層輸入之資料決定如何呼叫主流程
// Layer   : Executor
//
// Depend  :
//   CliRequestResolver
//   RTFProcessor
//   MyThread  
// Used by :
//   rtfconvert   
//
// Notes :
//   依據共用任務建構層給定的資料進行判斷如何呼叫主流程
// =====================================================

#pragma once

// forward declaration
namespace Conversion{
  struct  ResolvedConfig; 
}

namespace App{
  enum class AppExitCode : int{
    Success = 0,
    Fail = 1,
    CliError = 2,
    RunTimeError = 3,
    PartialSuccess = 4
  };

  class ConversionEngine{
  public:
  AppExitCode run(const Conversion::ResolvedConfig& RlConfig);
  private:
  }; 
}
