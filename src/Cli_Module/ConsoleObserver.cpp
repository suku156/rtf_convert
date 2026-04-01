// =====================================================
// Module : ConsoleObserver (implementation)
// =====================================================
#include "ConsoleObserver.h"
#include <iostream>

void ConsoleObserver::onLog(const std::wstring& msg){
   std::wcout << L"Cli 觀察者模式測試: "+ msg + L"\n";
}

void ConsoleObserver::onProgress(size_t done,size_t total){

}
