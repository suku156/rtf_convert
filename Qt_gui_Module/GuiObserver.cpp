#include "GuiObserver.h"
#include "Feedback_Module/ProgressEvent.h"
#include <QString>
#include <QDebug>

GuiObserver::GuiObserver(QObject* parent) : QObject(parent){}

void GuiObserver::onEvent(const ProgressEvent& event){
    QString prefix;

    switch (event.type) {
      case ProgressEventType::Start:
        prefix = "Gui [START]: ";
        break;
      case ProgressEventType::Info:
        prefix = "Gui [INFO]: ";
        break;
      case ProgressEventType::Finish:
        prefix = "Gui [FINISH]: ";
        break;
      case ProgressEventType::Warring:
        prefix = "Gui [WARRING]: ";
        break;
      case ProgressEventType::Error:
        prefix = "Gui [ERROR]: ";
        break;
      case ProgressEventType::BatchStart:
        prefix = "Gui [BATCHSTART]: ";
        emit progressChanged(static_cast<int>(event.done),static_cast<int>(event.total));
        break;
      case ProgressEventType::UnitDone:
        {
          QString text = QString("Gui [UNITDONE]: 已完成 %1 / %2 ")
                          .arg(static_cast<int>(event.done))
                          .arg(static_cast<int>(event.total));
          emit progressChanged(static_cast<int>(event.done),static_cast<int>(event.total));
          emit logMessage(text);
          return;
        }
      case ProgressEventType::BatchFinish:
        prefix = "Gui [BATCHFINISH]: ";
        emit progressChanged(static_cast<int>(event.done),static_cast<int>(event.total));
        break;
      default:
        prefix = "Gui [UNKNOW]";
        break;
    }

    QString eventStr  = prefix + QString::fromStdWString(event.message);
    emit logMessage(eventStr);
}

void GuiObserver::onProgress(const ProgressEvent& event){

}