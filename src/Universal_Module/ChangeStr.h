// =====================================================
// 共用的轉換 std::string 與 std::wstring 的轉換工具
// =====================================================
#pragma once
#include<string>

namespace ChangeStr {
  std::string wstringToUtf8(const std::wstring& message);  
}