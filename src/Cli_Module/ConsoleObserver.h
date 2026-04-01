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
#include <string>
#include <cstddef>

class ConsoleObserver : public IProgressObserver{
public:
   void onLog(const std::wstring& msg) override;
   void onProgress(size_t done,size_t total) override;
};