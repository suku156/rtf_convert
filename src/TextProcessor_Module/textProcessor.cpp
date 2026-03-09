// =====================================================
// Module : textProcessor (implementation)
// =====================================================
#include "textProcessor.h"
#include "LogSystem_Module/LogSystem.h"
#include "Universal_Module/Console.h"
#include<string>
#include<string_view>
#include<cstddef>
#include<algorithm>
#include<cstring>
#include<vector>
#include <utility>

// 唯一公開入口的函式定義
void textRtfProcessor::Processor(std::string& Cleaned,logSystem& logger){
    size_t before = Cleaned.size();
    
    controlGroupProcessor(Cleaned);
    pardPartClean(Cleaned);
    controlSymbolChange(Cleaned);
    finalRtfSymbolClean(Cleaned);
    compressBlankLines(Cleaned,3);
    removeOuterBraces(Cleaned);
    trimLeadingNewlines(Cleaned);
    removeProtectionSymbol(Cleaned);

    // 等到確定為可輸出資料(std::string) 後 清除 0x00 以防止意外
    {
      auto before = Cleaned.size();
      Cleaned.erase(std::remove(Cleaned.begin(), Cleaned.end(), '\0') ,Cleaned.end());

      auto removed = before - Cleaned.size();
      if (removed > 0) {
        logger.log(LogLevel::Warn,"已移除 " + std::to_string(removed) + " 個 NUL (0x00) 字元");
      }
    }

    size_t after = Cleaned.size();
    if(before != after){
      logger.log(LogLevel::Info,
                 std::string("文字清理完成，長度變化: ") +
                 std::to_string(before) + 
                 std::string(" → ") +
                 std::to_string(after));
    }else{
      logger.log(LogLevel::Warn,"控制符清理字數沒有變化");
    }
}
//用來確認傳入的字串的指定範圍內的byte 都在ASCII的範圍內
bool textRtfProcessor::checkAsciiScope(const std::string& text, size_t pos ,size_t len,bool tailOnly){
    if(tailOnly){
      if(pos >= text.size()) return false;
      size_t end = std::min(pos + len + 3 , text.size());
    
      for(size_t i = pos; i<end;i++){
        unsigned char c = static_cast<unsigned char>(text[i]);
        
        if(c == '\n' || c == '\r') return true;
        
        if(c >= 0x80){
          return false;
        }
      }
      return true;
    }
    
    size_t start = (pos >= 3)? pos -3 : 0;
    size_t end = std::min(pos + len + 3 , text.size());
    
    for(size_t i = start; i<end;i++){
      unsigned char c = static_cast<unsigned char>(text[i]);
      if(c >= 0x80){
        return false;
      }
    }
    return true;
}
//用來確認是否是接續的控制符以此來達到控制符區塊的終點
bool textRtfProcessor::isControlCharacter(char c){
  // 針對可能的中文,多位元符號遇到就認定為碰到非控制符
   unsigned char uc = static_cast<unsigned char>(c);
   if(uc >= 0x80) return false;
   
   if(uc == '\\' || uc == ' ' || uc == '\n' || uc == '\r' || 
     (uc >= 'A' && uc <= 'Z') ||
     (uc >= 'a' && uc <= 'z') ||
     (uc >= '0' && uc <= '9')){
    return true;
   }
   
   return false;
}
//可刪群組驗證工具
bool textRtfProcessor::start_with(std::string_view s,const char* prefix){
  size_t n = std::char_traits<char>::length(prefix);
  if(s.size() < n) return false;
  return std::memcmp(s.data(),prefix,n) == 0;
}
//用來判斷是不是已知的可刪群組
bool textRtfProcessor::isKnownDroppableGroup(std::string_view g){
    return start_with(g,"{\\fonttbl")    ||
           start_with(g,"{\\colortbl")   ||
           start_with(g,"{\\stylesheet") ||
           start_with(g,"{\\info")       ||
           start_with(g,"{\\*");
           
}
bool textRtfProcessor::isHex(char c){
  return (c >= '0' && c <= '9') ||
         (c >= 'a' && c <= 'f') ||
         (c >= 'A' && c <= 'F');
}
// 用來查找其中是否有可見文字
bool textRtfProcessor::hasVisibleText(std::string_view g){
    for(size_t i = 0; i < g.size(); ++i){
      unsigned char c = (unsigned char)g[i];

      // ASCII 可顯示
      if(c >= 0x20 && c < 0x7F) return true;

      // UTF-8
      if(c >= 0x80) return true;

      // RTF escape 文字 /'XX 或 \uXXXX 保險用
      
      if(c == '\\'){
        if(i + 3 < g.size() &&
         g[i+1] == '\'' &&
         isHex(g[i+2]) &&
         isHex(g[i+3])){
         return true;
        }

        if(i + 2 < g.size() &&
          g[i+1] == 'u' &&(g[i+2] == '-' || (g[i+2] >= '0' && g[i+2] <= '9'))){
          return true;
        }
      }
      
    }
    return false;
}
//用來判斷 群組內是 正文還是可以刪除的控制符段落
textRtfProcessor::GroupDecision textRtfProcessor::classifyGroup(std::string_view group){
    
    // 圖片群組 前面已經有處理 防呆一下
    if(start_with(group, "{\\pict") || start_with(group, "{\\*\\imgblock")){
       return GroupDecision::KeepAll;
    }
    
    // 遇到格式固定之群組直接刪
    if(isKnownDroppableGroup(group)){
      return GroupDecision::Drop;
    }
    
    // 只要遇到有可見文字的 -> 假定為正文
    if(hasVisibleText(group)){
      return GroupDecision::KeepText;
    }

    return GroupDecision::Drop;
}
//清理控制群組
void textRtfProcessor::controlGroupProcessor(std::string& Cleaned){
    std::string result;
    result.reserve(Cleaned.size());
    
    std::string target = "{\\rtf";
    size_t targetPos = Cleaned.find(target);
    if(targetPos == std::string::npos) return;

    size_t pos = targetPos + target.size();
    result.append(Cleaned,targetPos,pos);
    
    for(size_t i = pos;i<Cleaned.size();){
      unsigned char c = static_cast<unsigned char>(Cleaned[i]);
      
      if(c < 0x80 && c == '{' && i+1 < Cleaned.size()){
        unsigned char c1 = static_cast<unsigned char>(Cleaned[i+1]);
        if(c1 < 0x80 && c1 == '\\'){
          
          // 圖片標記群組特別處理
          if(i+12 < Cleaned.size() && Cleaned.compare(i,12,"{\\*\\imgblock") == 0){
            size_t end = i+2;
            int depth = 1;

            while(end < Cleaned.size() && depth > 0){
              unsigned char ec = static_cast<unsigned char>(Cleaned[end]);
              if(ec < 0x80 && ec == '{'){
                ++depth;
              }else if(ec < 0x80 && ec == '}'){
                --depth;
              }
              end++;
            }

            if(depth != 0){
              Console::ensureWcerr(L"[警告] 檔案中有大括號不平衡導致無法順利執行\n");
              return;
            }else{ // 直接不留群組
              size_t markPos = Cleaned.find("[[IMG_",i);
              size_t markEnd = Cleaned.find("]]",markPos);
              if (markPos != std::string::npos && markEnd != std::string::npos) {
                result.append(Cleaned.substr(markPos, markEnd - markPos + 2));
              }
              i = end;
              continue;
            }
          }

          size_t end = i+2;
          int depth = 1;
          
          while(end < Cleaned.size() && depth > 0){
            unsigned char ec = static_cast<unsigned char>(Cleaned[end]);
            if(ec < 0x80 && ec == '{'){
              ++depth;
            }else if(ec < 0x80 && ec == '}'){
              --depth;
            }
            end++;
          }
          
          if(depth != 0){
            Console::ensureWcerr(L"[警告] 檔案中有大括號不平衡導致無法順利執行\n");
            return;
          }else{
            // 需要增加判斷來表是可不可以跳過
            std::string_view innerView{Cleaned.data()+ i ,end - i};
            switch(classifyGroup(innerView)){
              case GroupDecision::Drop :
              i = end;
              break;
              case GroupDecision::KeepText :
              result.append(Cleaned, i + 1, end - i - 2);
              i = end;
              break;
              case GroupDecision::KeepAll :
              result.append(Cleaned, i, end - i);
              i = end;
              break;
            }
            continue;
          }
        }
      }

      result.push_back(Cleaned[i]);
      i++;
    }

    Cleaned.swap(result);
}
//清理 pard 控制段落
void textRtfProcessor::pardPartClean(std::string& Cleaned){
    std::string result;
    result.reserve(Cleaned.size());
    std::string target = "\\pard";

    for(size_t i = 0;i<Cleaned.size();){
      if(i + target.size() < Cleaned.size() && Cleaned.compare(i,target.size(),target) == 0){
        if(checkAsciiScope(Cleaned,i,target.size())){
          size_t end = i + target.size();
          while(end < Cleaned.size()){
            if(isControlCharacter(Cleaned[end])){
              end++;
            }else{
              break;
            }
          }
          i = end;
          continue;
        }
      }

      result.push_back(Cleaned[i]);
      i++;
    }

    Cleaned.swap(result);
}
// 用來置換rtf常見的功能符號
void textRtfProcessor::controlSymbolChange(std::string& Cleaned){
    std::string result;
    result.reserve(Cleaned.size());
   
    const std::vector<std::pair<std::string,std::string>> rtfReplacements = {
      {"\\par", "\n"},
      {"\\line", "\n"},
      {"\\tab", "\t"},
      {"\\cell", "\t"},
      {"\\row", "\n"},
      {"\\emdash", "—"},
      {"\\bullet", "•"}
    };

    size_t i = 0;
    while(i < Cleaned.size()){
      bool matched = false;

      for(const auto& [key,value] : rtfReplacements){
        if(i + key.size() <= Cleaned.size() && 
           Cleaned.compare(i,key.size(),key) == 0&&
           checkAsciiScope(Cleaned,i,key.size(),true))
        {
          result += value;
          i += key.size();
          matched = true;
          break;
        }
      }
      
      if(!matched){
        result.push_back(Cleaned[i]);
        i++;
      }
    }

    Cleaned.swap(result);
}
// 用來將剩餘的 rtf 控制符全部清空
void textRtfProcessor::finalRtfSymbolClean(std::string& Cleaned){
    std::string result;
    result.reserve(Cleaned.size());

    for(size_t i=0;i<Cleaned.size();){
      
      if(Cleaned[i] == '\\' && checkAsciiScope(Cleaned,i,1,true)){
        size_t end = i + 1;
        while(end < Cleaned.size()){
          unsigned char c = static_cast<unsigned char>(Cleaned[end]);
          if(c < 0x80){
            if(std::isdigit(c) || std::isalpha(c)){
              end++;
            }else{
              break;
            }
          }else{
            break;
          }
        }
        i = end;
        continue;
      }

      result.push_back(Cleaned[i]);
      i++;
    }
    Cleaned.swap(result);
}
//如果遇到過多連續空行,將其改為只有一行,用後方參數來設定最多可以空幾行
void textRtfProcessor::compressBlankLines(std::string& Cleaned,int maxBlankLines){
    std::string result;
    result.reserve(Cleaned.size());

    int newlineCount = 0;
    bool lineHasContent = false;

    for(size_t i = 0;i<Cleaned.size();){
      char c = Cleaned[i];

      if(c == '\r' && i + 1 < Cleaned.size() && Cleaned[i + 1] == '\n'){ // windows 特有的 \r\n
        if(!lineHasContent){
          newlineCount++;
        }else{ // 如果旗子為真代表遇到真正的文字段落
          newlineCount = 1; 
          lineHasContent = false;
        }
        if(newlineCount <= maxBlankLines){
          result.append("\r\n");
        }

        i+=2;

      }else if(c == '\n'){
        if(!lineHasContent){
          newlineCount++;
        }else{ 
          newlineCount = 1; 
          lineHasContent = false;
        }
        
        if(newlineCount <= maxBlankLines){
          result.push_back('\n');
        }

        i++;

      }else if(c == ' ' || c == '\t'){ // 遇到空格不要影響空行計算
        result.push_back(c);
        i++;
      }else{ // 遇到真正的文字段落就啟動標記
        lineHasContent = true;
        newlineCount = 0; 
        result.push_back(c);
        i++;
      }
    }

    Cleaned.swap(result);
}
//針對開頭與結尾可能多於的大括號
void textRtfProcessor::removeOuterBraces(std::string& Cleaned){
    if (Cleaned.empty()) return;

    //先定位最外層的大括號的位置(如果前後有多餘空格忽略它)
    size_t start = 0;
    while(start < Cleaned.size() && std::isspace(static_cast<unsigned char>(Cleaned[start]))){
      start++;
    }
    size_t end = Cleaned.size();
    while(end > start && 
         (std::isspace(static_cast<unsigned char>(Cleaned[end - 1])) ||
          Cleaned[end -1] == '\0')){
      end--;
    }
   
    //防呆 如果不是定位到最外圍的大括號就停止
    if(end - start < 2 || Cleaned[start] != '{' || Cleaned[end-1] != '}') return;

    //確認整體大括號處於平衡狀態
    int depth = 0;
    
    for (size_t i = start; i < end; ++i) {
        if (Cleaned[i] == '{') depth++;
        else if (Cleaned[i] == '}') {
            depth--;
            if (depth == 0 && i != end - 1) {
                // 代表外層在結尾前就關閉了，不是整體包覆
                return;
            }
        }
    }

    if(depth == 0){
      std::string result = Cleaned.substr(start +1 ,(end -1) - (start + 1));
      Cleaned.swap(result);
    }else{
      return; // 代表不平衡
    }

}
//清除開頭多餘的換行符
void textRtfProcessor::trimLeadingNewlines(std::string& Cleaned){
    size_t pos = Cleaned.find_first_not_of("\r\n");
    if(pos == std::string::npos){
      return;
    }else if(pos > 0){
      Cleaned.erase(0,pos);
    }
}
//等清理完成,移除保護符號
void textRtfProcessor::removeProtectionSymbol(std::string& Cleaned) {
    Cleaned.erase(std::remove(Cleaned.begin(), Cleaned.end(), '\x01'), Cleaned.end());
}

