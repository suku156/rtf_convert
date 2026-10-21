#include "Console.h"
#include "ChangeStr.h"
#include<string>
#include<mutex>
#include<iostream>

namespace {
  std::mutex g_consoleMutex;// 此cpp檔內唯一的鎖,用來確保 thread 時 std::wcout 不會被穿插
}
// 安全型 std::wcout
void Console::ensureWcout(const std::wstring& msg){
  std::lock_guard<std::mutex> lock(g_consoleMutex);
  #ifdef _WIN32
    std::wcout << msg << L"\n"; 
  #else
    std::cout  << ChangeStr::wstringToUtf8(msg) << "\n";
  #endif
}

// 安全型 std::wcerr
void Console::ensureWcerr(const std::wstring& msg){
  std::lock_guard<std::mutex> lock(g_consoleMutex);
  
  #ifdef _WIN32
    std::wcerr << msg << L"\n";
  #else
    std::cerr  << ChangeStr::wstringToUtf8(msg) << "\n";
  #endif
}