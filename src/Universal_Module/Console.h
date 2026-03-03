#pragma once
#include<string>

namespace Console{
  void ensureWcout(const std::wstring& msg);
  void ensureWcerr(const std::wstring& msg);  
}