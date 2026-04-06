#include "GuiObserver.h"
#include "Feedback_Module/ProgressEvent.h"
#include <QDebug>

void GuiObserver::onEvent(const ProgressEvent& event){
    qDebug() << "[GuiObserver] onLog called:"
             << QString::fromStdWString(event.message);
}

void GuiObserver::onProgress(const ProgressEvent& event){

}