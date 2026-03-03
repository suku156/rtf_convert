#include "Console.h"
#include<string>
#include<mutex>
#include<iostream>

namespace {
  std::mutex g_consoleMutex;// 此cpp檔內唯一的鎖,用來確保 thread 時 std::wcout 不會被穿插
}
// 安全型 std::wcout
void Console::ensureWcout(const std::wstring& msg){
  std::lock_guard<std::mutex> lock(g_consoleMutex);
  std::wcout << msg << L"\n"; 
}

// 安全型 std::wcerr
void Console::ensureWcerr(const std::wstring& msg){
  std::lock_guard<std::mutex> lock(g_consoleMutex);
  std::wcerr << msg << L"\n";
}