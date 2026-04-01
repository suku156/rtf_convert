// =====================================================
// Module  : IProgressObserver
// Author  : suku156
// Purpose : 給予訂閱者繼承用之介面
// Layer   : Feedback
//
// Depend  :
//   
// Used by :
//   ConsoleObserver
//   GuiObserver
//
// Notes :
//   給予訂閱者模式訂閱者用於繼承之共同介面
// =====================================================
#pragma once
#include <string>

class IProgressObserver{
public:
  virtual ~IProgressObserver() = default;
  virtual void onLog(const std::wstring& msg) = 0;
  virtual void onProgress(size_t done,size_t total) = 0;
}; 