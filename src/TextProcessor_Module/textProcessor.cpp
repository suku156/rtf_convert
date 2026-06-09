// =====================================================
// Module : textProcessor (implementation)
// =====================================================
#include "textProcessor.h"
#include "LogSystem_Module/LogSystem.h"
#include "Feedback_Module/ProgressEvent.h"
#include "Feedback_Module/IProgressObserver.h"
#include "RtfGroupProcessor_Module/FieldGroupProcessor.h"
#include "RtfGroupProcessor_Module/StartDestinationProcessor.h"
#include<string>
#include<string_view>
#include<cstddef>
#include<algorithm>
#include<cstring>
#include<vector>
#include <utility>
#include <unordered_map>

// 公開入口的函式定義
void textRtfProcessor::Processor(std::string& Cleaned,logSystem& logger){
    size_t before = Cleaned.size();
    
    
    controlGroupProcessor(Cleaned,logger);
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
  if (pos >= text.size()) return false;

  size_t start = tailOnly ? pos : ((pos >= 3) ? pos - 3 : 0);
  size_t end = std::min(pos + len, text.size());

  for (size_t i = start; i < end; ++i) {
    unsigned char c = static_cast<unsigned char>(text[i]);
    if (c >= 0x80) {
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
           start_with(g,"{\\themedata")        ||
           start_with(g,"{\\colorschememapping") ||
           start_with(g,"{\\xmlnstbl")         ||
           start_with(g,"{\\rsidtbl"); 
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

      // 跳過大括號
      if(c == '{' || c == '}'){
        ++i;
        continue;
      }

      // 遇到控制符
      if(c == '\\'){
        // escaped literal: \\ \{ \}
        if(i + 1 < g.size() &&
          (g[i + 1] == '\\' ||
           g[i + 1] == '{' ||
           g[i + 1] == '}'))
        {
          return true;
        }

        // ANSI hex escape: \'XX
        if(i + 3 < g.size() &&
           g[i + 1] == '\'' &&
           isHex(g[i + 2]) &&
           isHex(g[i + 3]))
        {
          return true;
        }

        // Unicode escape: \uN
        if(i + 2 < g.size() &&
           g[i + 1] == 'u' &&
           (g[i + 2] == '-' ||
            std::isdigit(static_cast<unsigned char>(g[i + 2]))))
        {
          return true;
        }

        // control word: \word-123?
        if(i + 1 < g.size() &&
            std::isalpha(static_cast<unsigned char>(g[i + 1])))
        {
          size_t j = i + 2;

          while(j < g.size() &&
                std::isalpha(static_cast<unsigned char>(g[j])))
          {
            ++j;
          }

          if(j < g.size() && g[j] == '-') {
            ++j;
          }

          while(j < g.size() &&
                std::isdigit(static_cast<unsigned char>(g[j])))
          {
            ++j;
          }

          if(j < g.size() && g[j] == ' ') {
              ++j;
          }

          i = j;
          continue;
        }

        // control symbol: \~ \- \_ \* 等
        i += 2;
        continue;
      }
      // 空白類不算正文
      if(std::isspace(c)){
        ++i;
        continue;
      }
        
      // 分號在 fonttbl 常出現，單獨不建議算正文
      if(c == ';'){
        ++i;
        continue;
      }

      // 走到這裡，才比較像真正正文
      return true;
    }
  return false;    
}
//用來判斷 群組內是 正文還是可以刪除的控制符段落
textRtfProcessor::GroupDecision textRtfProcessor::classifyGroup(std::string_view group){
  
  // 圖片群組 前面已經有處理 防呆一下
    if(start_with(group, "{\\pict") || start_with(group, "@@RTF_IMAGE_")){
       return GroupDecision::KeepAll;
    }
    // 圖片標記符保護
    if(group.find("@@RTF_IMAGE_") != std::string_view::npos ||
       group.find("@@RTF_IMAGE_ERR_") != std::string_view::npos)
    {
      return GroupDecision::KeepAll;
    }

    // word 特別蜂巢群組
    if(start_with(group, "{\\field")){
      return GroupDecision::Field;
    }

    // 遇到星號特殊群組
    if(start_with(group,"{\\*\\")){
      return GroupDecision::Start;
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
void textRtfProcessor::controlGroupProcessor(std::string& Cleaned,logSystem& logger){
    std::string result;
    result.reserve(Cleaned.size());
    
    std::string target = "{\\rtf";
    size_t targetPos = Cleaned.find(target);
    if(targetPos == std::string::npos) return;

    size_t pos = targetPos + target.size();
    result.append(Cleaned,targetPos,pos);
    
    for(size_t i = pos;i<Cleaned.size();){
      unsigned char c = static_cast<unsigned char>(Cleaned[i]);
      bool pictmark = false;

      if(c == '\\' && isSkippableRtfEscape(Cleaned,i)){
        result.push_back(Cleaned[i]);
        result.push_back(Cleaned[i + 1]);
        i += 2;
        continue;
      }

      if(c < 0x80 && c == '{'){
        size_t controlPos = i + 1;
        while (controlPos < Cleaned.size() &&
               std::isspace(static_cast<unsigned char>(Cleaned[controlPos]))) {
          ++controlPos;
        }  
        unsigned char c1 = static_cast<unsigned char>(Cleaned[controlPos]);
        if(c1 < 0x80 && c1 == '\\'){
          
          size_t end = i+2;
          int depth = 1;
          
          while(end < Cleaned.size() && depth > 0){
            unsigned char ec = static_cast<unsigned char>(Cleaned[end]);
            // 跳過 escaped brace，例如 \{ 或 \}
            // 跳過 escaped brace，例如 \{ 或 \}
            if(ec == '\\' && end + 1 < Cleaned.size() &&
              (Cleaned[end + 1] == '{' || Cleaned[end + 1] == '}'))
            {
              end += 2;
              continue;
            }
            
            if(ec < 0x80 && ec == '{'){
              ++depth;
            }else if(ec < 0x80 && ec == '}'){
              --depth;
            }
            end++;
          }
          
          if(depth != 0){
            notify(ProgressEvent{
              ProgressEventType::Warning,
              L"檔案中有大括號不平衡導致無法正確清理全部控制符"
            });
            return;
          }
         
          //偵測有無圖片標記符 要特別處理

          std::string_view groupView{Cleaned.data() + i, end - i};
          constexpr std::string_view imgHead = "@@RTF_IMAGE_";
          constexpr std::string_view imgTail   = "@@";
          
          size_t imgPos = groupView.find(imgHead);

          if(imgPos != std::string_view::npos){
            size_t imgEnd = groupView.find(imgTail, imgPos + imgHead.size());

            if(imgEnd != std::string_view::npos){
              imgEnd += imgTail.size();
              result.append(groupView.substr(imgPos, imgEnd - imgPos));
              i = end;
              continue;
            }
          }
         
          // 需要增加判斷來表是可不可以跳過
          std::string_view innerView{Cleaned.data()+ i ,end - i};
          switch(classifyGroup(innerView)){
            case GroupDecision::Drop :
              i = end;
              continue;
            case GroupDecision::KeepText :
              {
                std::string_view groupInner{Cleaned.data() + i + 1,end - i - 2};
                std::string cleanStr  = processGroupInner(groupInner,logger);
                result.append(cleanStr);
                i = end;
                continue;
              }
            case GroupDecision::KeepAll :
             result.append(Cleaned, i, end - i);
             i = end;
             continue;
            case GroupDecision::Field :
            {
              // field 群組由專用模組處理；
              // 無論是否產生文字，都不再走一般文字清理流程
              fieldGroupProcessor processor;
              if (auto text = processor.groupProcessor(innerView)) {
                result.append(*text);
              }
              i = end;
              continue;
            }
            case GroupDecision::Start :
            {
              // 星號特別群組交由專用模組處裡
              StartGroupProcessor startProcessor;
              StarGroupResult sg = startProcessor.groupProcessor(innerView);

              if(sg.unknown){
                logger.log(
                  LogLevel::Info,
                  "發現未在處裡名單上之星號群組 : " + 
                  sg.unknownGroupName +
                  " 已清理"
                );
              }

              if(sg.text){
                result.append(*sg.text);
              }

              i = end;
              continue;
            }
          }
        }
      }

      result.push_back(Cleaned[i]);
      i++;
    }

    Cleaned.swap(result);
}
// 用來將剩餘的 rtf 控制符全部清空
void textRtfProcessor::finalRtfSymbolClean(std::string& Cleaned){
    std::string result;
    
    const std::unordered_map<std::string,std::string> rtfReplacements = {
      {"par", "\n"},
      {"line", "\n"},
      {"tab", "\t"},
      {"emdash", "—"},
      {"bullet", "•"}
    };

    result.reserve(Cleaned.size());

    for(size_t i=0;i<Cleaned.size();){
      if(Cleaned[i] == '\\'){
        
        // 跳過 \\ \{ \} 
        if (i + 1 < Cleaned.size() &&
           (Cleaned[i + 1] == '\\' ||
            Cleaned[i + 1] == '{' ||
            Cleaned[i + 1] == '}'))
        {
          result.push_back(Cleaned[i + 1]);
          i += 2;
          continue;
        }

        // 找出控制符英文名稱
        size_t j = i + 1;
        while (j < Cleaned.size() &&
               std::isalpha(static_cast<unsigned char>(Cleaned[j])))
        {
          ++j;
        }
        
        std::string_view word(Cleaned.data() + i + 1,j - (i + 1));

        

        if(!word.empty()){
          // 名單內的控制符就替換掉
          auto it = rtfReplacements.find(std::string(word));
          if(it != rtfReplacements.end()){
            result += it->second;
          }

          //吃掉 -號開頭
          if (j < Cleaned.size() && Cleaned[j] == '-') ++j;
          while (j < Cleaned.size() &&
                 std::isdigit(static_cast<unsigned char>(Cleaned[j])))
          {
            ++j;
          }
          
          // 吃掉控制符空格
          if (j < Cleaned.size() && Cleaned[j] == ' ') ++j;

          i = j;
          continue;
        }

        // 非名單內的\開頭保留原樣，避免誤吃
        result.push_back(Cleaned[i]);
        ++i;
        continue;
      }
      // 一般字正常吃
      result.push_back(Cleaned[i]);
      ++i;
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
   
    if(end -start < 2) return;
    if(Cleaned[start] != '{' || Cleaned[end-1] != '}') return;
    
    Cleaned = Cleaned.substr(start + 1, end - start - 2);
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

// 用來判斷是否是 \{ \} \\ 等等 控制符
bool textRtfProcessor::isSkippableRtfEscape(const std::string& s, size_t pos){
  if(pos >= s.size() || s[pos] != '\\') return false;
  if(pos + 1 >= s.size()) return false;
  
  char next = s[pos + 1];

  switch (next) {
    case '{':
    case '}':
    case '\\':
    return true;
  }

  return false;
}

// 用在 controlGroupProcessor 流程中清理群組中的 {\*\ 群組
std::string textRtfProcessor::processGroupInner(std::string_view g,logSystem& logger){
  constexpr std::string_view fieldTarget = "\\field"; 
  if(g.size() >= fieldTarget.size() &&
     g.compare(0, fieldTarget.size(), fieldTarget) == 0)
  {
    return extractFldResultText(g);
  }
  
  std::string result;
  constexpr std::string_view target = "{\\*\\";
  
  result.reserve(g.size());

  for(size_t i =0;i<g.size();i++){
    if(i+ target.size() <= g.size() &&
       g.compare(i,target.size(),target) == 0)
    {
      int depth = 0;
      size_t j = i;
      bool closed = false;

      while(j < g.size()){
        char c = g[j];
        
        if(c == '\\' && j + 1 < g.size() &&
                   (g[j + 1] == '{' ||
                    g[j + 1] == '}' ||
                    g[j + 1] == '\\'))
        {
          j += 2;
          continue;
        }
        
        if(c == '{'){
          depth++;
        }else if( c == '}'){
          depth--;
        }
        
        j++;
        
        if(depth == 0){
          closed = true;
          break;
        } 
      }

      if(closed){
        std::string_view starGroupView = g.substr(i, j - i);
        StartGroupProcessor startProcessor;
        StarGroupResult sg = startProcessor.groupProcessor(starGroupView);

        if(sg.unknown){
          logger.log(
            LogLevel::Info,
            "發現未在處裡名單上之星號群組 : " + 
            sg.unknownGroupName +
            " 已清理"
          );
        }

        if(sg.text){
          result.append(*sg.text);
        }
        
        
        i = j - 1;
        continue;
      }
     
      result.push_back(g[i]);
      continue;
    }
    result.push_back(g[i]); 
  }

  std::string tmp = cleanRtfControlWordsButKeepSemantic(result);

  return replaceSemanticControls(tmp);
}

std::string textRtfProcessor::cleanRtfControlWordsButKeepSemantic(std::string_view input){
  std::string result;
  result.reserve(input.size());

  for(size_t i = 0; i < input.size(); ){
    if(input[i] != '\\'){
      
      // 群組的 {} 也丟掉
      if(input[i] == '{' || input[i] == '}'){
        ++i;
        continue;
      }
      
      result.push_back(input[i]);
      ++i;
      continue;
    }

    // 保護正文跳脫
    if(i + 1 < input.size() &&
        (input[i + 1] == '\\' ||
        input[i + 1] == '{' ||
        input[i + 1] == '}'))
    {
      result.push_back(input[i + 1]);
      i += 2;
      continue;
    }

    size_t j = i + 1;
    while(j < input.size() &&
          std::isalpha(static_cast<unsigned char>(input[j])))
    {
      ++j;
    }

    std::string_view word(input.data() + i + 1, j - (i + 1));

    // 語意控制符先保留，交給 replaceSemanticControls()
    if(word == "par" || word == "line" ||
       word == "tab" || word == "emdash" || word == "bullet")
    {
      result.append(input.data() + i, j - i);
      i = j;
      continue;
    }

    // 其他合法 control word：吃掉 word + 參數 + delimiter
    if(!word.empty()){
      if(j < input.size() && input[j] == '-') ++j;

      while(j < input.size() &&
            std::isdigit(static_cast<unsigned char>(input[j])))
      {
        ++j;
      }

      if(j < input.size() && input[j] == ' ') ++j;

      i = j;
      continue;
    }

    result.push_back(input[i]);
    ++i;
  }

  return result;
}

std::string textRtfProcessor::extractFldResultText(std::string_view fieldGroup){
  constexpr std::string_view target = "{\\fldrslt";

  size_t pos = fieldGroup.find(target);
  if(pos == std::string_view::npos){
    return {};
  }

  size_t j = pos;
  int depth = 0;
  bool closed = false;

  while(j < fieldGroup.size()){
    char c = fieldGroup[j];

    if(c == '\\' && j + 1 < fieldGroup.size() &&
       (fieldGroup[j + 1] == '{' ||
        fieldGroup[j + 1] == '}' ||
        fieldGroup[j + 1] == '\\'))
    {
      j += 2;
      continue;
    }

    if(c == '{'){
      ++depth;
    }else if(c == '}'){
       --depth;
    }

    ++j;

    if(depth == 0){
      closed = true;
      break;
    }
  }

  if(!closed){
    return {};
  }

  size_t realStart = pos + target.size();

  if(j <= realStart + 1){
    return {};
  }

  auto bodyView = fieldGroup.substr(realStart, j - realStart - 1);
  if(!bodyView.empty() && bodyView.front() == ' '){
    bodyView.remove_prefix(1);
  }
  
  return std::string(bodyView);
}

// processGroupInner 函式清理完後檢查並替換關鍵控制符
std::string textRtfProcessor::replaceSemanticControls(std::string_view target){
  std::string result;
  result.reserve(target.size());
  
  constexpr std::string_view parMark = "\\par"; 
  constexpr std::string_view lineMark = "\\line";
  
  // 防止誤判 \pard \linex0 等等部份重名控制符
  auto isControlEnd = [](std::string_view s,size_t pos) -> bool{
    return pos >= s.size() ||
           (!std::isalpha(static_cast<unsigned char>(s[pos])) && 
            !std::isdigit(static_cast<unsigned char>(s[pos])));
  };

  // 吃掉關鍵控制符後方的空格(如果有的話)
  auto skipControlDelimiter = [](std::string_view s, size_t& i) {
    if (i + 1 < s.size() &&
        (s[i + 1] == ' ' || s[i + 1] == '\r' || s[i + 1] == '\n')) 
    {
      ++i;
    }
  };

  for(size_t i = 0;i < target.size();i++){
    
    if(i + parMark.size() <= target.size() &&
       target.compare(i,parMark.size(),parMark) == 0 &&
       isControlEnd(target,i + parMark.size()))
    {
      result.append("@@par@@");
      i += (parMark.size() - 1); // 迴圈會自己 +1 所以這裡 -1
      skipControlDelimiter(target,i); // 有配合 -1 做調整
      continue;
    }

    if(i + lineMark.size() <= target.size() &&
       target.compare(i,lineMark.size(),lineMark) == 0 &&
       isControlEnd(target,i + lineMark.size()))
    {
      result.append("@@line@@");
      i += (lineMark.size() - 1); // 迴圈會自己 +1 所以這裡 -1
      skipControlDelimiter(target,i); // 有配合 -1 做調整
      continue;
    }

    char c = target[i];
    if(c == '\n' || c == '\r'){
      continue;
    }

    result.push_back(c);
  }

  return result;
}


void textRtfProcessor::notify(const ProgressEvent& event){
  if(observer_){
    observer_->onEvent(event);
  }
}