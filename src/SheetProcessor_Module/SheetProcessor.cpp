// =====================================================
// Module : SheetProcessor (implementation)
// =====================================================

#include "SheetProcessor.h"
#include <string>
#include <string_view>
#include <vector>
#include <cstddef>
#include <cctype>


void sheetProcessor::processor(std::string& rtfContent){
  std::vector<sheetScope> pending;
  std::string target = "\\trowd";
  std::string relaytarget = "\\intbl";
  size_t pos = rtfContent.find(target);
  if(pos == std::string::npos){
    return;
  }

  while(pos != std::string::npos){
    sheetScope sheetscope;
    sheetscope.sheetstart_ = pos;
    
    size_t targetSize = 0;
    if(rtfContent.compare(pos,target.size(),target) == 0){
      sheetscope.newsheetStr_.append("@@RTF_TABLE_TROWD@@");
      targetSize = target.size();
    }
    else if(rtfContent.compare(pos,relaytarget.size(),relaytarget) == 0){
      sheetscope.newsheetStr_.append("@@RTF_TABLE_INTBL@@");
      targetSize = relaytarget.size();
    }else{
      break;
    }
    
    
    size_t end = pos;
    bool foundRow = false;
   
    for(size_t i = pos + targetSize; i < rtfContent.size();i++){
       char c = rtfContent[i];
       
        if(i + 4 <= rtfContent.size() && 
          rtfContent.compare(i,4,"\\row") == 0 &&
          isControlEnd(rtfContent,i+4))
        {
           sheetscope.newsheetStr_.append("@@RTF_TABLE_ROW@@");
           end = i + 4;
           foundRow = true;
           break;
        }
       
        if( i + 5 <= rtfContent.size() &&
           rtfContent.compare(i,5,"\\cell") == 0 &&
           isControlEnd(rtfContent,i+5))
        {
           sheetscope.newsheetStr_.append("@@RTF_TABLE_CELL@@");
           i+=4;
           continue;
        }

        if(c == '{' && i +1 <rtfContent.size() && rtfContent[i+1] == '\\'){
            size_t j = i;
            int depth = 0;
            bool closed = false;
            
            while(j < rtfContent.size()){
              char ch = rtfContent[j];
              
              if (ch == '\\' && j + 1 < rtfContent.size() &&
                  (rtfContent[j + 1] == '{' ||
                   rtfContent[j + 1] == '}' ||
                   rtfContent[j + 1] == '\\')) 
              {
                j += 2;
                continue;
              }

              if(ch == '{'){
                depth++;
              }else if(ch == '}'){
                depth--;
              }
              
              j++;
              
              if(depth == 0){
                closed = true;
                break;
              } 
            }

            if(!closed) break;

            std::string_view groupView{rtfContent.data()+ i ,j - i};
            TableToken token = groupProcessor(groupView);
            bool groupHasRow = false;
           
            switch(token.type){
              case TableTokenType::text :
              sheetscope.newsheetStr_.append(token.text);
              break;
              case TableTokenType::cell :
              sheetscope.newsheetStr_.append(token.text);
              break;
              case TableTokenType::row :
              groupHasRow = true;
              sheetscope.newsheetStr_.append(token.text);
              break;
            }

            i = j - 1;
            if(groupHasRow){
              end = j;
              foundRow = true;
              break;
            }
        }
    }

    if(foundRow){
      sheetscope.sheetend_ = end;
      pending.push_back(std::move(sheetscope));
      pos = findNextTableEntry(rtfContent,end);
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

bool sheetProcessor::isControlEnd(std::string_view s,size_t pos){
  return pos >= s.size() ||
           !std::isalpha(static_cast<unsigned char>(s[pos]));
}

TableToken sheetProcessor::groupProcessor(std::string_view groupView){
  TableToken token;
  token.text.reserve(groupView.size());
  token.type = TableTokenType::text;// 預設是文字
  for(size_t i = 0;i<groupView.size();i++){
    
    if(i + 4 <= groupView.size() && 
       groupView.compare(i,4,"\\row") == 0 &&
       isControlEnd(groupView,i+4))
    {
      token.type = TableTokenType::row;
      token.text = "@@RTF_TABLE_ROW@@";
      break;
    }

    if(i + 5 <= groupView.size() &&
       groupView.compare(i,5,"\\cell") == 0 &&
       isControlEnd(groupView,i+5))
    {
      token.type = TableTokenType::cell;
      token.text = "@@RTF_TABLE_CELL@@";
      break;
    }

    char c = groupView[i];
    if(c == '{' || c == '}') continue;

    if(c == '\\'){
      
      // 處理 \'XX，例如 \'a4\'a4
      if (i + 3 < groupView.size() &&
          groupView[i + 1] == '\'' &&
          std::isxdigit(static_cast<unsigned char>(groupView[i + 2])) &&
          std::isxdigit(static_cast<unsigned char>(groupView[i + 3])))
      {
        token.text.push_back('\\');
        token.text.push_back('\'');
        token.text.push_back(groupView[i + 2]);
        token.text.push_back(groupView[i + 3]);

        i += 3;
        continue;
      }

      
      i++;
     
      while(i < groupView.size()&& std::isalpha(static_cast<unsigned char>(groupView[i]))){
        i++;
      }

      if (i < groupView.size() &&
        (groupView[i] == '-' || std::isdigit(static_cast<unsigned char>(groupView[i]))))
      {
        if (groupView[i] == '-') {
          ++i;
        }

        while (i < groupView.size() &&
          std::isdigit(static_cast<unsigned char>(groupView[i]))) 
        {
          ++i;
        }
      }

      if (i <groupView.size() && groupView[i] == ' ') {
        continue;
      }

      // 目前 i 已經在下一個非控制字字元，讓 for 不要跳過它
      if(i > 0) i--;
      continue;
    }

    token.text.push_back(c);
  }

  return token;
}

size_t sheetProcessor::findNextTableEntry(const std::string& rtfContent,size_t from){
  size_t trowdPos = rtfContent.find("\\trowd",from);
  size_t intblPos = rtfContent.find("\\intbl",from);

  if(trowdPos == std::string::npos) return intblPos;
  if(intblPos == std::string::npos) return trowdPos;

  return std::min(trowdPos,intblPos);
}