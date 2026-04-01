#include "GuiObserver.h"
#include <string>
#include <cstddef>
#include <QDebug>

void GuiObserver::onLog(const std::wstring& msg){
    qDebug() << "[GuiObserver] onLog called:"
             << QString::fromStdWString(msg);
}

void GuiObserver::onProgress(size_t done,size_t total){

}