#pragma once

#include<array>
#include<string_view>
#include<ostream>

namespace ErrorSystem {
  
  enum class ErrorLevel{
    None,       // 無錯
    Warning,    // 警告(不影響執行)
    Recoverable,// 影響某個區塊,但可以繼續執行流程
    FatalLocal, // 區域型致命錯誤,需中斷當下目標之執行(非整個程式)
    FatalGlobal // 全域型致命錯誤,需直接中止整個程式
  };

  extern const std::array<const char*,5> ErrorLevelNames;

  std::ostream& operator<<(std::ostream& os,ErrorLevel level);
  
  constexpr std::string_view getLevelStr(ErrorLevel level) noexcept{
    auto index = static_cast<size_t>(level);
    if(index < ErrorLevelNames.size()){
      return ErrorLevelNames[index];
    }

    return "UnKnowErrorLevel";
  }


}