// =====================================================
// Module : ConsoleObserver (implementation)
// =====================================================
#include "ConsoleObserver.h"
#include "Feedback_Module/ProgressEvent.h"
#include "Universal_Module/Console.h"


void ConsoleObserver::onLog(const ProgressEvent& event){
   Console::ensureWcout(event.message);
}

void ConsoleObserver::onProgress(const ProgressEvent& event){

}
