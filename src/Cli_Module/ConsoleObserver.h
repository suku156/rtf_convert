// =====================================================
// Module  : ConsoleObserver
// Author  : suku156
// Purpose : 給予訂閱者繼承用之介面
// Layer   : Cli
//
// Depend  :
//   IProgressObserver
// Used by :
//   ConversionExecutor
//   ConversionTaskBuilder
//   rtfconvert
//
// Notes :
//   給予訂閱者模式訂閱者用於繼承之共同介面
// =====================================================
#pragma once
#include "Feedback_Module/IProgressObserver.h"

struct  ProgressEvent;

class ConsoleObserver : public IProgressObserver{
public:
   void onEvent(const ProgressEvent& event) override;
   void onProgress(const ProgressEvent& event) override;
};