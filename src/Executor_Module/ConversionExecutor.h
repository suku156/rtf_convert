// =====================================================
// Module  : ConversionExecutor
// Author  : suku156
// Purpose : 依據任務建構層輸入之資料決定如何呼叫主流程
// Layer   : Executor
//
// Depend  :
//   ConversionTaskBuilder
//   RTFProcessor
//   MyThread  
// Used by :
//   rtfconvert   
//
// Notes :
//   依據共用任務建構層給定的資料進行判斷如何呼叫主流程
// =====================================================

#pragma once
#include <Task_Module/ConversionTask.h>
#include <Feedback_Module/IProgressObserver.h>

// forward declaration
/*
namespace Conversion{
  struct  ResolvedConfig; 
}
*/


namespace App{
  enum class AppExitCode : int{
    Success = 0,
    Fail = 1,
    RunTimeError = 2,
    PartialSuccess = 3
  };

  class ConversionEngine{
  public:
  AppExitCode run(const BuildResult& result,IProgressObserver* observer = nullptr);
  private:
  }; 
}
