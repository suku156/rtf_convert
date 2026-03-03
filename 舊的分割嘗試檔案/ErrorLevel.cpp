#include "ErrorLevel.h"

namespace ErrorSystem{
  
  const std::array<const char*,5> ErrorLevelNames = {
    "None",
    "Warning",
    "Recoverable",
    "FatalLocal",
    "FatalGlobal"
  };

  std::ostream& operator<< (std::ostream& os , ErrorLevel level){
    auto index = static_cast<size_t>(level);
    if(index < ErrorLevelNames.size()){
      return os << ErrorLevelNames[index]; 
    }
    return os << "Unknown";
  }
}