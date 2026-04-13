// =====================================================
// Module  : GuiObserver
// Author  : suku156
// Purpose : 負責gui接收核心觀察者傳遞訊息並做出反應的模組
// Layer   : gui
//
// Depend  :
//  IProgressObserver
//  mainwindow
//
// Used by :
//   mainwindow
//   ConversionExecutor
//   ConversionTaskBuilder
//
// Notes :
//   gui 中主要負責接收並決定如和處裡核心傳遞內容的模組
//   使用 Qt signal/slot 傳遞訊息給 mainwindow
// =====================================================

#ifndef GUIOBSERVER_H
#define GUIOBSERVER_H

#include "Feedback_Module/IProgressObserver.h"
#include <qobject.h>

struct ProgressEvent;

class GuiObserver : public QObject,public IProgressObserver{
Q_OBJECT
public:
    explicit GuiObserver(QObject* parent = nullptr);
    void onEvent(const ProgressEvent& event) override;
    void onProgress(const ProgressEvent& event) override;
signals:
    void logMessage(QString text);
    void progressChanged(int done,int total);
};

#endif // GUIOBSERVER_H
