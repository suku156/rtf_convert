// ======================================================
// 共用小工具
// Notes :
//   確保使用這個小工具的模組在使用不會因為多執行緒被穿插
//   支援 std::wcout 與 std::wcerr
// ==============================
#pragma once
#include<string>

namespace Console{
  void ensureWcout(const std::wstring& msg);
  void ensureWcerr(const std::wstring& msg);  
}