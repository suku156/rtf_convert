// =====================================================
// Module : SheetProcessor (implementation)
// =====================================================

#include "SheetProcessor.h"
#include "RtfGroupProcessor_Module/FieldGroupProcessor.cpp"
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

        if(c == '{'){
          size_t controlPos = i + 1;
          while (controlPos < rtfContent.size() &&
                 std::isspace(static_cast<unsigned char>(rtfContent[controlPos]))) {
            ++controlPos;
          }  
          
          if(controlPos < rtfContent.size() && rtfContent[controlPos] == '\\'){
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
            if(groupView.find("{\\field") != std::string_view::npos){
              fieldGroupProcessor fieldgroupprocessor;
              fieldgroupprocessor.groupProcessor(groupView);
            }
            bool groupHasRow = false;
            TableToken token = groupProcessor(groupView);
            
            if(!token.text.empty()){
              sheetscope.newsheetStr_.append(token.text);
            }
            
            if(token.hasRow){
              groupHasRow = true;
            }
              

            i = j - 1;
            if(groupHasRow){
              end = j;
              foundRow = true;
              break;
            }
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

// 函式多載 const std::string& 版本
bool sheetProcessor::isControlEnd(const std::string& s,size_t pos){
  return pos >= s.size() ||
           !std::isalpha(static_cast<unsigned char>(s[pos]));
}

// 函式多載 std::string_view 版本
bool sheetProcessor::isControlEnd(std::string_view s,size_t pos){
  return pos >= s.size() ||
           !std::isalpha(static_cast<unsigned char>(s[pos]));
}


// 用來偵測群組中有無表格相關的關鍵控制符
TableToken sheetProcessor::groupProcessor(std::string_view groupView){
  TableToken token;
  token.text.reserve(groupView.size());
  token.type = TableTokenType::text;// 預設是文字

  constexpr std::string_view ROWMARK = "\\row";
  constexpr std::string_view CELLMARK = "\\cell";

  auto skipControlDelimiter = [](std::string_view s, size_t& i) {
    while (i + 1 < s.size() &&
           (s[i + 1] == ' ' || s[i + 1] == '\r' || s[i + 1] == '\n')) {
        ++i;
    }
  };

  for(size_t i = 0;i<groupView.size();i++){
    
    if(i + 4 <= groupView.size() && 
       groupView.compare(i,ROWMARK.size(),ROWMARK) == 0 &&
       isControlEnd(groupView,i + ROWMARK.size()))
    {
      token.hasRow = true;
      token.type = TableTokenType::row;
      token.text += "@@RTF_TABLE_ROW@@";
      
      i += (ROWMARK.size() -1); // 迴圈會自己 +1 所以這裡 -1
      skipControlDelimiter(groupView,i);//吃掉關鍵控制符號方的空格

      continue;
    }

    if(i + 5 <= groupView.size() &&
       groupView.compare(i,CELLMARK.size(),CELLMARK) == 0 &&
       isControlEnd(groupView,i + CELLMARK.size()))
    {
      token.hasCell = true;
      token.type = TableTokenType::cell;
      token.text += "@@RTF_TABLE_CELL@@";
      
      i += (CELLMARK.size() -1); // 迴圈會自己 +1 所以這裡 -1
      skipControlDelimiter(groupView,i);//吃掉關鍵控制符號方的空格
      
      continue;
    }

    char c = groupView[i];

    // 忽略換行符
    if(c == '\n' || c == '\r') continue;

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

      // 保留關鍵 \ucX 控制符
      if(i + 3 < groupView.size() && 
         groupView[i + 1] == 'u'  &&
         groupView[i + 2] == 'c'  &&
         std::isdigit(static_cast<unsigned char>(groupView[i+3])))
      {
        size_t j = i+3;
        
        token.text.push_back('\\');
        token.text.push_back('u');
        token.text.push_back('c');
        
        while (j < groupView.size() &&
              std::isdigit(static_cast<unsigned char>(groupView[j])))
        {
          token.text.push_back(groupView[j]);
          ++j;
        }

        i = j - 1; // 迴圈自己 +1
        continue;
      }

      // 處裡 \uXXXX utf 標記符
      if(i + 1 < groupView.size() && groupView[i + 1] == 'u'){
        size_t j = i + 2;

        //可能有負號
        if(j < groupView.size() && groupView[j] == '-'){
          j++;
        }

        size_t digitStart = j;
        while(j < groupView.size() &&
              std::isdigit(static_cast<unsigned char>(groupView[j])))
        {
          j++;
        }

        if( j > digitStart){
          token.text.append(groupView.substr(i,j-i));
          i = j - 1;
          continue;
        }
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