#include "RtfPreProcessor.h"
#include "LogSystem_Module/LogSystem.h"
#include<string>
#include<string_view>
#include<optional>

namespace{
  bool isControlEnd(std::string_view s,size_t pos){
    return pos >= s.size() ||
           !std::isalpha(static_cast<unsigned char>(s[pos]));
  }
}

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
            if (auto preserved = extractPreservedContent(currentGroup)) {
              result.append(*preserved);
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

          if(name == "nonesttables"){
            logger_.log(
              LogLevel::Warn,
              "找到漏網的 nonesttables 群組，已移除"
            );
            i = end;
            continue;
          }

          if(name == "nesttableprops"){
            logger_.log(
              LogLevel::Error,
              "漏網的 nesttableprops 群組，已錯過表格控制符轉換階段，整組移除"
            );

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
       name == "bkmkend" ||
       name == "revtbl" ||
       name == "expandedcolortbl" ||
       name == "htmltag") 
    {
      return PreDecodeGroupAction::Drop;
    }

    if(name == "nesttableprops"){
      return PreDecodeGroupAction::Special;
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
     name == "mmathPr" ) 
  {
    return PreDecodeGroupAction::Drop;
  }

  if(name == "field" ||
     name == "pict" ||
     name == "shp" ||
     name == "object" ||
     name == "nonesttables") 
  {
    return PreDecodeGroupAction::Special;
  }

  return PreDecodeGroupAction::Recurse;
}
// 用來判斷有無圖片標記符
PreImageMarkerInfo RtfPreProcessor::containsImageMarker(std::string_view currentGroup){
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
// 判斷群組中是否包含 Table marker。
// 若包含，回傳完整群組，以保留已預處理的表格資訊。
std::optional<std::string> 
RtfPreProcessor::containsTableMarker(std::string_view group){
  constexpr std::string_view prefix = "@@RTF_TABLE_";
  constexpr std::string_view suffix = "@@";

  size_t pos = 0;

  while ((pos = group.find(prefix, pos)) != std::string_view::npos) {

    size_t cur = pos + prefix.size();

    if (cur >= group.size()) {
        return std::nullopt;
    }

    // 找 marker 結尾
    size_t end = group.find(suffix, cur);

    if (end == std::string_view::npos) {
      return std::nullopt;
    }

    // prefix 後至少要有內容
    if (end == cur) {
      pos += prefix.size();
      continue;
    }

    return std::string(group);
  }

  return std::nullopt;
}


// 負責清理  shp 群組中的 shptxt 中不用的控制符並找出要轉換的 目標控制符
std::string RtfPreProcessor::processShapeTextRtf(std::string_view group)
{
  constexpr std::string_view targetPar  = "\\par";
  constexpr std::string_view targetLine = "\\line";
  
  std::string result;
  result.reserve(group.size());

  for (size_t i = 0; i < group.size(); ++i) {
    unsigned char c = static_cast<unsigned char>(group[i]);

    // 跳過群組符號
    if (c == '{' || c == '}') {
      continue;
    }

    // 特別的標記符目標
    if(i + targetPar.size() <= group.size() &&
       group.compare(i,targetPar.size(),targetPar) == 0 &&
       isControlEnd(group,i + targetPar.size()))
    {
      result.append("@@RTF_SHAPE_TEXT_PAR@@");
      (i += targetPar.size()) - 1; // 迴圈會自動+1 因此此處需要校正
      continue;
    }

    if(i + targetLine.size() <= group.size() &&
       group.compare(i,targetLine.size(),targetLine) == 0 &&
       isControlEnd(group,i + targetLine.size()))
    {
      result.append("@@RTF_SHAPE_TEXT_LINE@@");
      (i += targetLine.size()) - 1; // 迴圈會自動+1 因此此處需要校正
      continue;
    }

    if (c == '\\') {
      // 先看是不是控制符號
      if (i + 1 >= group.size()) {
        break;
      }

      // 保存並跳過 escaped \{ \} 
      if(c == '\\' && i + 1 < group.size() &&
        (group[i + 1] == '{' ||
         group[i + 1] == '}' ||
         group[i + 1] == '\\'))
      {
        result.push_back(group[i]);
        result.push_back(group[i+1]);
        i++; // 迴圈會自動加所以這裡只加一
        continue;
      }

      // 處理 \'XX，例如 \'a4\'a4
      if (i + 3 < group.size() &&
          group[i + 1] == '\'' &&
          std::isxdigit(static_cast<unsigned char>(group[i + 2])) &&
          std::isxdigit(static_cast<unsigned char>(group[i + 3])))
      {
        result.push_back('\\');
        result.push_back('\'');
        result.push_back(group[i + 2]);
        result.push_back(group[i + 3]);

        (i += 3) - 1;
        continue;
      }

      // 保留關鍵 \ucX 控制符
      if(i + 3 < group.size() && 
         group[i + 1] == 'u'  &&
         group[i + 2] == 'c'  &&
         std::isdigit(static_cast<unsigned char>(group[i+3])))
      {
        size_t j = i+3;
        
        result.push_back('\\');
        result.push_back('u');
        result.push_back('c');
        
        while (j < group.size() &&
              std::isdigit(static_cast<unsigned char>(group[j])))
        {
          result.push_back(group[j]);
          ++j;
        }

        i = j - 1; // 迴圈自己 +1
        continue; 
      }

      // 處裡 \uXXXX utf 標記符
      if(i + 1 < group.size() && group[i + 1] == 'u'){
        size_t j = i + 2;

        //可能有負號
        if(j < group.size() && group[j] == '-'){
          j++;
        }

        size_t digitStart = j;
        while(j < group.size() &&
              std::isdigit(static_cast<unsigned char>(group[j])))
        {
          j++;
        }

        if(j > digitStart){
          result.append(group.substr(i,j-i));
          i = j - 1;
          continue;
        }
      }

      // 非必要的控制符部份
      ++i;

      // 跳過 control word 的英文字母
      while (i + 1 < group.size()) {
        unsigned char wc = static_cast<unsigned char>(group[i + 1]);
        if (!std::isalpha(wc)) {
           break;
        }
        ++i;
      }

      // 跳過控制字後面的數值
      if (i + 1 < group.size() &&
         (group[i + 1] == '-' ||std::isdigit(static_cast<unsigned char>(group[i + 1]))))
      {
        ++i;

        while (i + 1 < group.size() &&
               std::isdigit(static_cast<unsigned char>(group[i + 1])))
        {
          ++i;
        }
      }

      // 控制符後可能存在一個 delimiter space
      if (i + 1 < group.size() &&group[i + 1] == ' '){
        ++i;
      }

      continue;
    }

    result.push_back(group[i]);
  }

  return result;
}

// 用來在 shp 群組中擷取出關鍵控制符資訊
std::optional<std::string> 
RtfPreProcessor::extractShapeProperties(std::string_view group){
  
  std::string result;
  result.reserve(group.size());

  // 目標控制符名稱
  constexpr std::string_view targetShpLeft   = "\\shpleft";
  constexpr std::string_view targetShpRight  = "\\shpright";
  constexpr std::string_view targetShpTop    = "\\shptop";
  constexpr std::string_view targetShpBottom = "\\shpbottom";
  constexpr std::string_view targetShpZ      = "\\shpz";

  // 共用的擷取負號與數字的 lambda
  auto getnum = [&result,&group](std::string& marker,size_t j) -> size_t{
    
    if(j < group.size() && group[j] == '-'){
      marker.push_back('-');
      j++;
    }

    size_t digitStart = j;

    while(j < group.size() &&std::isdigit(static_cast<unsigned char>(group[j])))
    {
      marker.push_back(group[j]);
      ++j;
    }

    // 至少要有一位數字
    if(j > digitStart){
      marker += "@@";
      result += marker;
    }

    return j;
  };

  for(size_t i = 0 ;i < group.size(); i++){
    
    if(i + targetShpLeft.size() <= group.size() &&
       group.compare(i,targetShpLeft.size(),targetShpLeft) == 0 &&
       isControlEnd(group,i + targetShpLeft.size()))
    {
      std::string marker = "@@RTF_SHPLEFT:";
      
      size_t j = i + targetShpLeft.size();

      j = getnum(marker,j);
      
      i = j - 1; // 迴圈會自動加一
      continue;
    }

    if(i + targetShpRight.size() <= group.size() &&
       group.compare(i,targetShpRight.size(),targetShpRight) == 0 &&
       isControlEnd(group,i + targetShpRight.size()))
    {
      std::string marker = "@@RTF_SHPRIGHT:";
      
      size_t j = i + targetShpRight.size();

      j = getnum(marker,j);
      
      i = j - 1; // 迴圈會自動加一
      continue;
    }

    if(i + targetShpTop.size() <= group.size() &&
       group.compare(i,targetShpTop.size(),targetShpTop) == 0 &&
       isControlEnd(group,i + targetShpTop.size()))
    {
      std::string marker = "@@RTF_SHPTOP:";
      
      size_t j = i + targetShpTop.size();

      j = getnum(marker,j);
      
      i = j - 1; // 迴圈會自動加一
      continue;
    }

    if(i + targetShpBottom.size() <= group.size() &&
       group.compare(i,targetShpBottom.size(),targetShpBottom) == 0 &&
       isControlEnd(group,i + targetShpBottom.size()))
    {
      std::string marker = "@@RTF_SHPBOTTOM:";
      
      size_t j = i + targetShpBottom.size();

      j = getnum(marker,j);
      
      i = j - 1; // 迴圈會自動加一
      continue;
    }
    
    if(i + targetShpZ.size() <= group.size() &&
       group.compare(i,targetShpZ.size(),targetShpZ) == 0 &&
       isControlEnd(group,i + targetShpZ.size()))
    {
      std::string marker = "@@RTF_SHPZ:";
      
      size_t j = i + targetShpZ.size();

      j = getnum(marker,j);
      
      i = j - 1; // 迴圈會自動加一
      continue;
    }
  }
  
  if(result.empty()){
    return std::nullopt;
  }

  return result;
}

// 負責尋找並將 shp 群組中 shptxt 內容以及群組中需要的控制符資訊轉換為 marker
std::optional<std::string>
RtfPreProcessor::extractShapeTextMarker(std::string_view group)
{
  constexpr std::string_view target = "{\\shptxt";

  size_t pos = group.find(target);

  if (pos == std::string_view::npos) {
    return std::nullopt;
  }

  size_t end = findGroupEnd(group, pos);

  if (end == std::string_view::npos || end <= pos) {
    return std::nullopt;
  }

  std::string result;
  result.append("@@RTF_SHAPE_START@@");

  // 從完整段落擷取出文字方塊群組需要的標記符資訊
  auto properties = extractShapeProperties(group);

  if(properties){
    result.append(*properties);
  }

  // 先保留完整的 {\shptxt ... } 群組
  std::string_view shapeTextGroup = group.substr(pos, end - pos + 1);

  std::string cleaned = processShapeTextRtf(shapeTextGroup);

  if (cleaned.empty()) {
    return std::nullopt;
  }

  result += "@@RTF_SHAPE_TEXT_START@@";
  result += cleaned;
  result += "@@RTF_SHAPE_TEXT_END@@";

  result.append("@@RTF_SHAPE_END@@");
  
  return result;
}
// 負責 shp 群組的統一決策之入口
std::optional<std::string> 
RtfPreProcessor::extractPreservedContent(std::string_view group){
  // 已有圖片 marker
  auto imageMarker = containsImageMarker(group);

  if (imageMarker.hasMarker) {
    return imageMarker.marker;
  }

  // 已有表格 marker
  if (auto tableMarker = containsTableMarker(group)) {
    return tableMarker;
  }

  // 3. Shape 裡的 shptxt 以及 文字方塊所需之資訊
  if (auto shapeTextMarker = extractShapeTextMarker(group)) {
    return shapeTextMarker;
  }

  return std::nullopt;
}

