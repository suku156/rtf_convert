// =====================================================
// Module : ConsoleObserver (implementation)
// =====================================================
#include "ConsoleObserver.h"
#include "Feedback_Module/ProgressEvent.h"
#include "Universal_Module/Console.h"


void ConsoleObserver::onEvent(const ProgressEvent& event){
   switch(event.type){
      case ProgressEventType::Info:
      Console::ensureWcout(L"[INFO]: "  + event.message);
      return;
      case ProgressEventType::Start:
      Console::ensureWcout(L"[START]: "  + event.message);
      return;
      case ProgressEventType::Finish:
      Console::ensureWcout(L"[FINISH]: "  + event.message);
      return;
      case ProgressEventType::BatchStart:
      Console::ensureWcout(L"[BATCHSTART]: "  + event.message);
      return;
      case ProgressEventType::BatchFinish:
      Console::ensureWcout(L"[BATCHFINISH]: "  + event.message);
      return;
      case ProgressEventType::UnitDone:
      Console::ensureWcout(L"[UNITDONE]: "  + event.message
            + std::to_wstring(event.done) 
            +  L" / " + std::to_wstring(event.total));
      return;    
      case ProgressEventType::Warring:
      Console::ensureWcerr(L"[WARNING]: "  + event.message);
      return;
      case ProgressEventType::Error:
      Console::ensureWcerr(L"[ERROR]: "  + event.message);
      return;
      case ProgressEventType::Fail:
      Console::ensureWcerr(L"[FAIL]: "  + event.message);
      return;
      default:
      Console::ensureWcerr(L"[UNKNOWN_TYPE]");
      return;
   }
}

void ConsoleObserver::onProgress(const ProgressEvent& event){
  
}
