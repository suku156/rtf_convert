// =====================================================
// Module : SheetProcessor (implementation)
// =====================================================

#include "SheetProcessor.h"
#include <string>
#include <vector>
#include <cstddef>
#include <algorithm>

void sheetProcessor::processor(std::string& rtfContent){
  std::vector<sheetScope> pending;
  std::string target = "\\trowd";
  size_t pos = rtfContent.find(target);
  if(pos == std::string::npos) return;

  while(pos != std::string::npos){
    sheetScope sheetscope;
    sheetscope.sheetstart_ = pos;
    
    size_t end = pos;
    bool foundRow = false;

    for(size_t i = pos + target.size() ; i < rtfContent.size();i++){
       char c = rtfContent[i];
       
        if(i + 4 <= rtfContent.size() && 
          rtfContent.compare(i,4,"\\row") == 0 &&
          isControlEnd(rtfContent,i+4))
        {
           sheetscope.newsheetStr_.append("__RTF_TABLE_ROW__");
           end = i + 4;
           foundRow = true;
           break;
        }
       
        if( i + 5 <= rtfContent.size() &&
           rtfContent.compare(i,5,"\\cell") == 0 &&
           isControlEnd(rtfContent,i+5))
        {
           sheetscope.newsheetStr_.append("__RTF_TABLE_CELL__");
           i+=4;
           continue;
        }

        if(c == '{' && i +1 <rtfContent.size() && rtfContent[i+1] == '\\'){
            size_t j = i;
            int depth = 0;
            
            while(j < rtfContent.size()){
              char ch = rtfContent[j];
              
              if(ch == '{'){
                depth++;
              }else if(ch == '}'){
                depth--;
              }
              
              sheetscope.newsheetStr_.push_back(ch);  
              j++;
              
              if(depth == 0) break;
            }
            
            i = j - 1;
        }
    }

    if(foundRow){
      sheetscope.sheetend_ = end;
      pending.push_back(std::move(sheetscope));
      pos = rtfContent.find(target,end);
    }else{
      break; 
    }
    
  }

  // 使用 pending 陣列 由後往前替換區域文字為 struct 內的 std::string 成員
  for (auto it = pending.rbegin(); it != pending.rend(); ++it) {
    rtfContent.replace(
        it->sheetstart_,
        it->sheetend_ - it->sheetstart_,
        it->newsheetStr_
    );
   }
}

bool sheetProcessor::isControlEnd(const std::string& s,size_t pos){
  return pos >= s.size() ||
           !std::isalpha(static_cast<unsigned char>(s[pos]));
}