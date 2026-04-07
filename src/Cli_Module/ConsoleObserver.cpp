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
      case ProgressEventType::Warning:
      Console::ensureWcerr(L"[WARNING]: "  + event.message);
      return;
      case ProgressEventType::Error:
      Console::ensureWcerr(L"[ERROR]: "  + event.message);
      return;
      default:
      Console::ensureWcerr(L"[UNKNOWN_TYPE]");
      return;
   }
}

void ConsoleObserver::onProgress(const ProgressEvent& event){

}
