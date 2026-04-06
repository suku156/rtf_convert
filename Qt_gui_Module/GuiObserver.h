// =====================================================
// Module  : GuiObserver
// Author  : suku156
// Purpose : 負責gui接收核心觀察者傳遞訊息並做出反應的模組
// Layer   : gui
//
// Depend  :
//  IProgressObserver
//
// Used by :
//   mainwindow
//   ConversionExecutor
//   ConversionTaskBuilder
//
// Notes :
//   gui 中主要負責接收並決定如和處裡核心傳遞內容的模組
// =====================================================

#ifndef GUIOBSERVER_H
#define GUIOBSERVER_H

#include "Feedback_Module/IProgressObserver.h"

struct ProgressEvent;


class GuiObserver : public IProgressObserver{
public:
    void onLog(const ProgressEvent& event) override;
    void onProgress(const ProgressEvent& event) override;
};

#endif // GUIOBSERVER_H
