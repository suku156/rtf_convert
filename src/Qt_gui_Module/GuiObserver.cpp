#include "GuiObserver.h"
#include "Feedback_Module/ProgressEvent.h"
#include <QString>
#include <QDebug>

GuiObserver::GuiObserver(QObject* parent) : QObject(parent){}

void GuiObserver::onEvent(const ProgressEvent& event){
    QString prefix;

    switch (event.type) {
      case ProgressEventType::Start:
        prefix = "[CORE] [START]: ";
        break;
      case ProgressEventType::Info:
        prefix = "[CORE] [INFO]: ";
        break;
      case ProgressEventType::Finish:
        prefix = "[CORE] [FINISH]: ";
        break;
      case ProgressEventType::Warring:
        prefix = "[CORE] [WARRING]: ";
        break;
      case ProgressEventType::Error:
        prefix = "[CORE] [ERROR]: ";
        break;
      case ProgressEventType::BatchStart:
        prefix = "[CORE] [BATCHSTART]: ";
        emit progressChanged(static_cast<int>(event.done),static_cast<int>(event.total));
        break;
      case ProgressEventType::UnitDone:
        {
          QString text = QString("[CORE] [UNITDONE]: 已完成 %1 / %2 ")
                          .arg(static_cast<int>(event.done))
                          .arg(static_cast<int>(event.total));
          emit progressChanged(static_cast<int>(event.done),static_cast<int>(event.total));
          emit logMessage(text);
          return;
        }
      case ProgressEventType::BatchFinish:
        prefix = "[CORE] [BATCHFINISH]: ";
        emit progressChanged(static_cast<int>(event.done),static_cast<int>(event.total));
        break;
      case ProgressEventType::Fail:
        prefix = "[CORE] [FAIL]: ";
        break;
      default:
        prefix = "[CORE] [UNKNOWN]";
        break;
    }

    QString eventStr  = prefix + QString::fromStdWString(event.message);
    emit logMessage(eventStr);
}

void GuiObserver::onProgress(const ProgressEvent& event){

}