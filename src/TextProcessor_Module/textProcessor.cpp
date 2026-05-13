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
#include <unordered_set>

namespace{
  // rtf　最後可以直接移除的控制符之名單
  bool shouldRemoveRtfControlWord(std::string_view word){
    static const std::unordered_set<std::string_view> removeSet = {
      "b", "i", "ul", "ulnone","f", "fs", "cf","lang", "langfe","rtlch", "ltrch","hich", 
      "dbch", "loch","insrsid","af", "fcs","plain","pard","rtf","ansi","ansicpg","deff",
      "nouicompat","deflang","deflangfe","viewkind","uc","adeflang","adeff","stshfdbch",
      "stshfloch","stshfhich","stshfbi","themelang","themelangfe","themelangcs","mlMargin",
      "noqfpromote","mmathPr","mmathFont","mbrkBin","mbrkBinSub","msmallFrac","mdispDef",
      "mrMargin","mdefJc","mwrapIndent","mintLim","mnaryLim","paperw","paperh","marg",
      "margr","margt","margb","gutter","ltrsect","deftab","ftnbj","aenddoc","trackmoves",
      "trackformatting","donotembedsysfont",
      "relyonvml","donotembedlingdata","grfdocevents","validatexml","showplaceholdtext",
      "ignoremixedcontent","saveinvalidxml","showxmlerrors","formshade","horzdoc","dgmargin",
      "dghspace","dgvspace","dghorigin","dgvorigin","dghshow","dgvshow","jcompress","lnongrid",
      "viewscale","splytwnine","ftnlytwnine","htmautsp","useltbaln","alntblind","lytcalctblwd",
      "lyttblrtgr","lnbrkrule","nobrkwrptbl","snaptogridincell","allowfieldendsel","wrppunct",
      "asianbrkrule","rsidroot","newtblstyruls","nogrowautofit","usenormstyforlist",
      "noindnmbrts","felnbrelev","nocxsptable","indrlsweleven","noafcnsttbl","afelev",
      "utinl","hwelev","spltpgpar","notcvasp","notbrkcnstfrctbl","notvatxbx","krnprsnet",
      "cachedcolbal","upr","fet","nofeaturethrottle","ilfomacatclnup","ltrpar","sectd",
      "ltrsect","linex","headery","footery","colsx","endnhere","sectlinegrid","sectspecifyl",
      "sftnbj","ltrrow","afs","chshdng","chcfpat","chcbpat","ab","cs","chbrdr","brdrs",
      "brdrw","brdrcf1","brdrframe","chshdng","chcfpat","chcbpat","margl","noproof","brdrcf",
      "hyphauto","sbknone","sftnnar","saftnnrlc","sectunlocked","pgwsxn","pghsxn","marglsxn",
      "margrsxn","margtsxn","margbsxn","ftnstart","ftnrstcont","ftnnar","aftnrstcont","aftnstart",
      "aftnnrlc","pgndec","charrsid"
    };

    return removeSet.find(word) != removeSet.end();;
  }
}

// 公開入口的函式定義
void textRtfProcessor::Processor(std::string& Cleaned,logSystem& logger){
    size_t before = Cleaned.size();
    
    replaceShapeGroupsWithImageMarkers(Cleaned);
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
void textRtfProcessor::replaceShapeGroupsWithImageMarkers(std::string& Cleaned){
  size_t searchPos = 0;
  while(true){
    
    // 找找看有無目標群組開頭
    size_t pos = Cleaned.find("{\\shp",searchPos);
    if(pos == std::string::npos) break;
    
    size_t n = pos +1;
    int flag = 1;

    while(n < Cleaned.size()){
      
      if(Cleaned[n] == '\\' && n + 1 < Cleaned.size()){
        n += 2;   // 跳過 escaped token
        continue;
      }
      
      if(Cleaned[n] == '{'){
        flag++;
      }
      else if(Cleaned[n] == '}'){
        flag--;
      }
      n++;
      if(flag == 0) break;
    }
    
    if(flag != 0) break;
    // 拿取 {\shp 群組去做檢查
    std::string groupResult = Cleaned.substr(pos,n-pos);
    // 拿來記錄標記符 沒有就會為空直接替換掉 shape 群組
    std::string result;
    
    for(size_t i = 0; i < groupResult.size();++i){
      constexpr std::string_view target = "@@RTF_IMAGE_";
      if(groupResult.compare(i,target.size(),target) == 0){
        constexpr std::string_view tail = "@@";
       
        size_t symbolEnd = groupResult.find(tail,i + target.size());
        if (symbolEnd != std::string::npos) {
            symbolEnd += tail.size();
            result = groupResult.substr(i, symbolEnd - i);
        }
        break;
      }
    }

    // 執行替換的動作
    Cleaned.replace(pos,n-pos,result);

    // 更新搜尋的位置(從替換後的長度開始)
    searchPos = pos + result.size();
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
           start_with(g,"{\\*")          || 
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
    if(start_with(group, "{\\pict") || start_with(group, "@@RTF_IMAGE_")){
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
            Console::ensureWcerr(L"[WARRING]: 檔案中有大括號不平衡導致無法正確清理全部控制符");
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
                std::string cleanStr  = processGroupInner(groupInner);
                //result.append(Cleaned, i + 1, end - i - 2);
                result.append(cleanStr);
                i = end;
                continue;
              }
            case GroupDecision::KeepAll :
             result.append(Cleaned, i, end - i);
             i = end;
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
      if(i + target.size() <= Cleaned.size() && 
         Cleaned.compare(i,target.size(),target) == 0 &&
         checkAsciiScope(Cleaned,i,target.size()))
      {
        
        size_t end = i + target.size();
        while(end < Cleaned.size()){
          if(isControlCharacter(Cleaned[end])){
            end++;
          }else{
            break;
          }
        }
          
        // 吃掉控制符後方的空格
        if (end < Cleaned.size() && Cleaned[end] == ' ') {
          ++end;
        }

        i = end;
        continue;
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
      {"\\emdash", "—"},
      {"\\bullet", "•"}
    };

    // 用來避免對重名控制符誤判
    auto isControlWordBoundary = [](const std::string& s, size_t pos) -> bool {
      if (pos >= s.size()) {
        return true;
      }

      unsigned char c = static_cast<unsigned char>(s[pos]);

      // 後面仍是英文字母，代表控制字還沒結束，例如 \linex0
      if (std::isalpha(c)) {
        return false;
      }

      // 後面是數字或負號，可能是控制字參數，例如 \xxx123
      // 對這批「純符號替換」來說，保守起見不直接匹配
      if (std::isdigit(c) || c == '-') {
        return false;
      }

      return true;
    };

    size_t i = 0;
    while(i < Cleaned.size()){
      bool matched = false;

      for(const auto& [key,value] : rtfReplacements){
        if(i + key.size() <= Cleaned.size() && 
           Cleaned.compare(i,key.size(),key) == 0 &&
           checkAsciiScope(Cleaned,i,key.size(),true) &&
           isControlWordBoundary(Cleaned,i+key.size()))
        {
          result += value;
          i += key.size();

          // 用來吃掉控制符後方為空白的情況
          if (i < Cleaned.size() && Cleaned[i] == ' ') {
            ++i;
          }

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

        if(!word.empty() && shouldRemoveRtfControlWord(word)){
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
std::string textRtfProcessor::processGroupInner(std::string_view g){
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
        i = j - 1;
        continue;
      }
     
      result.push_back(g[i]);
      continue;
    }
    result.push_back(g[i]); 
  }

  return replaceSemanticControls(result);
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

// 此函式是給外部用的公開函式
std::string textRtfProcessor::removeIgnorableDestinations(std::string_view rtf){
  std::string result;
  result.reserve(rtf.size());

  const std::string target = "{\\*\\";
  const std::string imgMarker = "@@RTF_IMAGE";
  const std::string imgtail = "@@";
  auto findGroupEnd = [](std::string_view s, size_t start) -> size_t {
    int depth = 1;
    size_t j = start + 1;

    while (j < s.size() && depth > 0) {
      char c = s[j];

      if (c == '\\' && j + 1 < s.size()) {
        j += 2;
        continue;
      }

      if (c == '{') {
        ++depth;
      } else if (c == '}') {
        --depth;
      }

      ++j;
    }

    if (depth == 0) {
      return j;
    }

    return std::string_view::npos;
  };
    
  for (size_t i = 0; i < rtf.size(); ++i) {
    if (i + target.size() <= rtf.size() &&
          rtf.compare(i, target.size(), target) == 0)
    {
      size_t groupEnd = findGroupEnd(rtf,i);

      if(groupEnd != std::string_view::npos){
        std::string_view groupView{rtf.data() + i,groupEnd - i};

        size_t markerPos = groupView.find(imgMarker);

        if(markerPos != std::string_view::npos){
          size_t markerStart = i + markerPos;
          size_t markerTail  = groupView.find(imgtail,markerPos+imgMarker.size());
          size_t markerEnd;
          if(markerTail != std::string_view::npos){
            markerTail += imgtail.size();
            markerEnd = i + markerTail;
          }else{
            markerEnd = std::string_view::npos;
          }
          
          if (markerEnd != std::string_view::npos && markerEnd <= groupEnd)
          {
            result.append(rtf.data() + markerStart,markerEnd - markerStart);
          }
        }

        i = groupEnd - 1;
        continue;
      }
    }

    result.push_back(rtf[i]);
  }

  return result;
}