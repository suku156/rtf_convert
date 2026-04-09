#include "GuiObserver.h"
#include "Feedback_Module/ProgressEvent.h"
#include "mainwindow.h"
#include <QString>


void GuiObserver::onEvent(const ProgressEvent& event){
    if(!mainWindow_) return;
    switch (event.type) {
      case ProgressEventType::Start :
      {
        QString eventStr = "[START]: ";
        eventStr += QString::fromStdWString(event.message);
        mainWindow_ -> appendLog(eventStr);
        return;
      }
      case ProgressEventType::Info :
      {
        QString eventStr = "[INFO]: ";
        eventStr += QString::fromStdWString(event.message);
        mainWindow_ -> appendLog(eventStr);
        return;
      }
      case ProgressEventType::Finish :
      {
        QString eventStr = "[FINISH]: ";
        eventStr += QString::fromStdWString(event.message);
        mainWindow_ -> appendLog(eventStr);
        return;
      }
      case ProgressEventType::Warning :
      {
        QString eventStr = "[WARRING]: ";
        eventStr += QString::fromStdWString(event.message);
        mainWindow_ -> appendLog(eventStr);
        return;
      }
      case ProgressEventType::Error :
      {
        QString eventStr = "[ERROR]: ";
        eventStr += QString::fromStdWString(event.message);
        mainWindow_ -> appendLog(eventStr);
        return;
      }
      case ProgressEventType::BatchStart :
      {
        QString eventStr = "[BATCHSTART]: ";
        eventStr += QString::fromStdWString(event.message);
        mainWindow_ -> appendLog(eventStr);
        return;
      }
      case ProgressEventType::UnitDone :
      {
        QString eventStr = QString("[UNITDONE]: 已處理 %1 / %2")
                                .arg(static_cast<qulonglong>(event.done))
                                .arg(static_cast<qulonglong>(event.total));
        mainWindow_ -> appendLog(eventStr);
        return;
      }
      case ProgressEventType::BatchFinish :
      {
        QString eventStr = "[BATCHFINISH]: ";
        eventStr += QString::fromStdWString(event.message);
        mainWindow_ -> appendLog(eventStr);
        return;
      }
      defult:
      {
        QString eventStr = "[UNKNOWTYPE]";
        mainWindow_ -> appendLog(eventStr);
        return;
      }
    }
}

void GuiObserver::onProgress(const ProgressEvent& event){

}