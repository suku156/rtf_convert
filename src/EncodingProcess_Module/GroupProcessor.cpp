#include "GroupProcessor.h"
#include<string>
#include<cstddef>

// 判斷該群組中是否都是 utf 特殊符號 函式定義
bool utfSymbolJudgment::judgment(const std::string& content){
    for(size_t i = 0;i<content.size();++i){
      unsigned int cp = utf8ToCodepoint(content,i);

      //如果找到的範圍不再特殊符號區間的話
      if(!isAllowedSymbol(cp)) return false;
    }
    return true;
}
unsigned int utfSymbolJudgment::utf8ToCodepoint(const std::string& s, size_t& i){
    unsigned char c = s[i];
    if (c < 0x80) return c;

    if ((c >> 5) == 0x6 && i + 1 < s.size()) {
        unsigned int cp = ((c & 0x1F) << 6) | (s[i + 1] & 0x3F);
        i += 1;
        return cp;
    } else if ((c >> 4) == 0xE && i + 2 < s.size()) {
        unsigned int cp = ((c & 0x0F) << 12) |
                          ((s[i + 1] & 0x3F) << 6) |
                          (s[i + 2] & 0x3F);
        i += 2;
        return cp;
    } else if ((c >> 3) == 0x1E && i + 3 < s.size()) {
        unsigned int cp = ((c & 0x07) << 18) |
                          ((s[i + 1] & 0x3F) << 12) |
                          ((s[i + 2] & 0x3F) << 6) |
                          (s[i + 3] & 0x3F);
        i += 3;
        return cp;
    }

    return 0xFFFD; 
}
bool utfSymbolJudgment::isAllowedSymbol(unsigned int cp){
    return 
    (cp >= 0x2600 && cp <= 0x26FF) || 
    (cp >= 0x25A0 && cp <= 0x25FF) || 
    (cp == 0x0020) ||                 
    (cp == 0x3000) ||                 
    (cp == 0x0009) || (cp == 0x000A) || (cp == 0x000D);     
}

// utf8 用的群組處理類別 函式定義
void utf8GroupProcessor::processGroups(std::string& content){
    cleanTargetGroup(content,"{\\loch");
    cleanTargetGroup(content,"{\\hich");
    cleanTargetGroup(content,"{\\dbch");
    handleSpecialSymbolGroups(content);
}
void utf8GroupProcessor::cleanTargetGroup(std::string& content,const std::string& target){
    size_t pos = 0;
    while((pos = content.find(target,pos)) != std::string::npos){
      size_t end = content.find('}',pos);
      if(end == std::string::npos) break;

      //取出開頭與結尾範圍內的文字進行判斷與加工
      std::string group = content.substr(pos,end - pos + 1);

      //判斷擷取出的段落中有無 \uXXXX 這樣的控制符
      bool hasUnicode = group.find("\\u") != std::string::npos;//
      if(hasUnicode){ //如果有找到控制符,就跳過這個群組去找下一個目標
        pos = end + 1;
        continue; 
      }

      //去除群組中的控制符
      std::string cleaned;
      bool inCtrl = false;
      for(size_t i = 0;i<group.size();i++){
        char c = group[i];
        if(c == '\\'){
          inCtrl = true;
        }else if(inCtrl){ //滿足條件才判斷是非控制符,在記錄到字串中
          if(!std::isalpha(static_cast<unsigned char>(c)) && !std::isdigit(static_cast<unsigned char>(c))){
            inCtrl = false;
          }
        }else if(c != '{' && c != '}'){ // 大括號也不要
          cleaned.push_back(c);
        }
      }

      //如果清出來的字串的開頭或結尾有多餘的換行或空白就清除它
      while(!cleaned.empty() && (cleaned.front() == '\n' || cleaned.front() == '\r' || cleaned.front() == ' ')){
        cleaned.erase(cleaned.begin());//檢查字串第一個字是換行或空白就清除它,持續到不是
      }
      while(!cleaned.empty() && (cleaned.back() == '\n' || cleaned.back() == '\r' || cleaned.back() == ' ')){
        cleaned.pop_back();//檢查字串最後一個字是換行或空白就清除它,持續到不是
      }

      if(!cleaned.empty()){//處理完的群組的開頭都加上特殊符號防止誤刪
        char c = '\x01'; 
        cleaned.insert(cleaned.begin(),c);
      }

      // 確認條件都正確就進行替換的動作
      if(!cleaned.empty()){
        content.replace(pos,end - pos +1,cleaned);
        pos += cleaned.size();
      }else{
        content.erase(pos,end-pos+1);
      }
    }
}
void utf8GroupProcessor::handleSpecialSymbolGroups(std::string& content){
    size_t pos = 0;
    utfSymbolJudgment symboljudgment;
    while ((pos = content.find('{', pos)) != std::string::npos) {
      size_t end = content.find('}', pos + 1);
      if (end == std::string::npos) break;

      std::string group = content.substr(pos+1,end-pos-1);

      //如果清出來的字串的開頭或結尾有多餘的換行或空白就清除它
      while(!content.empty() && (content.front() == '\n' || content.front() == '\r' || content.front() == ' ')){
       content.erase(content.begin());//檢查字串第一個字是換行或空白就清除它,持續到不是
      }
      while(!content.empty() && (content.back() == '\n' || content.back() == '\r' || content.back() == ' ')){
        content.pop_back();//檢查字串最後一個字是換行或空白就清除它,持續到不是
      }


      if(symboljudgment.judgment(group)){
        std::string replaced = "\x01" + group;
        content.replace(pos,end-pos+1,replaced);
        pos += replaced.size();
      }else{
        pos = end+1;
      }
    }
}
