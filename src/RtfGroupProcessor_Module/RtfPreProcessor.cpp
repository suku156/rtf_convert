#include "RtfPreProcessor.h"
#include "LogSystem_Module/LogSystem.h"
#include<string>
#include<string_view>
#include<optional>

std::string RtfPreProcessor::process(std::string_view rtf){
    constexpr std::string_view target = "{\\rtf";
    size_t targetPos = rtf.find(target);
    if(targetPos == std::string_view::npos){
      return std::string(rtf);
    } 
  
    size_t pos = targetPos + target.size();
    
    std::string result;
    result.reserve(rtf.size());

    result.append(rtf.substr(0,pos));

    result.append(processView(rtf,pos));
    
    return result;
}
// 專門用來處裡遞迴策略需求的迴圈函式
std::string RtfPreProcessor::processView(std::string_view src,size_t pos){
  std::string result;
  result.reserve(src.size());

  for(size_t i = pos;i<src.size();){
    
    if(src[i] == '\\' && i + 1 < src.size()) {
      char next = src[i + 1];
      if (next == '{' || next == '}' || next == '\\') {
        result.push_back(src[i]);
        result.push_back(src[i+1]);
        i += 2;
        continue;
      }
    }
    
    if(src[i] == '{'){
      size_t end = findGroupEnd(src,i);

      if (end == std::string_view::npos) {
        logger_.log(LogLevel::Warn, "RTF 群組大括號不平衡");
        result.push_back(src[i]);
        ++i;
        continue;
      }

      std::string_view currentGroup(src.data() + i , end -i);
      auto head = parseGroupHead(currentGroup);
      auto action = getGroupAction(head);

      switch(action){
        case PreDecodeGroupAction::Drop :
        i = end;
        continue;
        case PreDecodeGroupAction::Recurse :
        result.append(processGroup(currentGroup));
        i = end;
        continue;
        case PreDecodeGroupAction::Keep :
        result.append(currentGroup);
        i = end;
        continue;
        case PreDecodeGroupAction::Special :
        {
          if(!head.name){
            result.append(processGroup(currentGroup));
            i = end;
            continue;
          }
          
          const auto& name = *head.name;

          if (name == "pict" || name == "shp" || name == "object") {
            auto markerInfo = containsMarker(currentGroup);
            
            if (markerInfo.hasMarker) {
              result.append(markerInfo.marker);
            }
            
            // 沒 marker 就整群丟掉
            i = end;
            continue;
          }

          if (name == "field") {
            // 先保留整個群組結構留給後續群組處理
            result.append(currentGroup);
            i = end;
            continue;
          }

          result.append(processGroup(currentGroup));
          i = end;
          continue;
        }

      }
    }
    result.push_back(src[i]);
    i++;
  }
  return result;
}
// 在遞迴判斷中用來處理需要判斷群組內部之函式
std::string RtfPreProcessor::processGroup(std::string_view group){
  if(group.size() < 2) return std::string(group);

  std::string result;

  result.push_back('{');

  std::string_view inner = group.substr(1, group.size() - 2);

  result.append(processView(inner));

  result.push_back('}'); 

  return result;
}

size_t RtfPreProcessor::findGroupEnd(std::string_view group,size_t pos){
  
  if(pos >= group.size() || group[pos] != '{'){
    return std::string_view::npos;
  }
  
  size_t end = pos;
  int depth = 0;
  while(end < group.size()){
    char c = group[end];

    if(c == '\\' && end + 1 < group.size()) {
      char next = group[end + 1];
      if (next == '{' || next == '}' || next == '\\') {
        end += 2;
        continue;
      }
    }
    
    if(c == '{'){
      depth++;
    }
    else if(c == '}'){
      depth--;
      if(depth == 0){
        return end + 1; // 群組後一格
      }
      if(depth < 0){
        return std::string_view::npos;// 群組不平衡
      }
    }
    end++;
  }

  return std::string_view::npos;
}
// 擷取群組開頭之資訊用作判斷
GroupHeadInfo RtfPreProcessor::parseGroupHead(std::string_view group){
  GroupHeadInfo info;
  if(group.empty() || group[0] != '{'){
    return info;
  }
  
  size_t pos = 1;

  while(pos < group.size() &&
        std::isspace(static_cast<unsigned char>(group[pos]))) 
  {
   ++pos;
  }

  if (pos >= group.size() || group[pos] != '\\') {
    return info;
  }
  pos++;

  if (pos < group.size() && group[pos] == '*') {
    info.isStar = true;
    ++pos;

    while (pos < group.size() &&
           std::isspace(static_cast<unsigned char>(group[pos]))) 
    {
      ++pos;
    }

    if (pos >= group.size() || group[pos] != '\\') {
      return info;
    }

    ++pos;
  }

  size_t start = pos;

  while(pos < group.size() &&
        std::isalpha(static_cast<unsigned char>(group[pos])))
  {
    pos++;
  }

  if(start == pos){
    return info;
  }
  
  std::string result(group.substr(start, pos -start));
  info.name = result;
  return info;
}

bool RtfPreProcessor::start_with(std::string_view s,std::string_view prefix){
    return s.size() >= prefix.size() &&
           s.compare(0,prefix.size(),prefix) == 0;
}
// 判斷群組開頭該如何處裡
PreDecodeGroupAction RtfPreProcessor::getGroupAction(GroupHeadInfo info){
  if (!info.name) {
    return PreDecodeGroupAction::Recurse;
  }

  const std::string& name = *info.name;
  
  if(info.isStar){
    if(name == "fchars" ||
       name == "lchars" ||
       name == "ud" ||
       name == "wgrffmtfilter" ||
       name == "pnseclvl" ||
       name == "rsidtbl" ||
       name == "themedata" ||
       name == "colorschememapping" ||
       name == "datastore" ||
       name == "latentstyles" ||
       name == "defchp" ||
       name == "defpap" ||
       name == "xmlnstbl" ||
       name == "mmathPr" ||
       name == "generator" ||
       name == "userprops" ||
       name == "ftnsep" ||
       name == "listtable" ||
       name == "listoverridetable" ||
       name == "bkmkstart" ||
       name == "bkmkend") 
    {
      return PreDecodeGroupAction::Drop;
    }

    return PreDecodeGroupAction::Recurse;
  }

  if(name == "fonttbl" ||
     name == "colortbl" ||
     name == "stylesheet" ||
     name == "info" ||
     name == "generator" ||
     name == "filetbl" ||
     name == "listtbl" ||
     name == "listoverridetable" ||
     name == "xmlnstbl" ||
     name == "rsidtbl" ||
     name == "mmathPr") 
  {
    return PreDecodeGroupAction::Drop;
  }

  if(name == "field" ||
     name == "pict" ||
     name == "shp" ||
     name == "object") 
  {
    return PreDecodeGroupAction::Special;
  }

  return PreDecodeGroupAction::Recurse;
}
// 用來判斷有無標記符
PreImageMarkerInfo RtfPreProcessor::containsMarker(std::string_view currentGroup){
  PreImageMarkerInfo info;

  constexpr std::string_view imgHead = "@@RTF_IMAGE";
  constexpr std::string_view imgErr  = "_ERR_";
  
  size_t imgPos = currentGroup.find(imgHead);
  if(imgPos == std::string_view::npos){
    return info;
  }

  std::size_t cur = imgPos + imgHead.size();
  if(cur >= currentGroup.size()) return info;

  if(cur + imgErr.size() <= currentGroup.size() &&
     currentGroup.compare(cur, imgErr.size(), imgErr) == 0)
  {
    cur += imgErr.size();
  }
  else if(currentGroup[cur] == '_'){
    cur++;
  }
  else{
    return info;
  }

  std::size_t start = cur;
  while(cur < currentGroup.size() &&
        std::isdigit(static_cast<unsigned char>(currentGroup[cur])))
  {
    ++cur;
  }

  // 至少要有一位數字
  if(start == cur){
    return info;
  }

  if(cur + 1 >= currentGroup.size() ||
     currentGroup[cur] != '@' ||
     currentGroup[cur + 1] != '@')
  {
    return info;
  }
  
  info.hasMarker = true;
  info.marker = std::string(currentGroup.substr(imgPos, (cur+2) - imgPos));
  
  return info;
}
