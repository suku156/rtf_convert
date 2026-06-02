#include "FieldGroupProcessor.h"
#include <string>
#include <string_view>
#include <optional>
#include <cstddef>
#include <cstring>
#include <array>
#include <iostream>
#include <fstream>

/*
void dumpStringViewToFile(std::string_view sv, const std::string& path)
{
  std::ofstream out(path, std::ios::binary);
  if(!out) return;
  out.write(sv.data(), static_cast<std::streamsize>(sv.size()));
}
*/
std::optional<std::string> fieldGroupProcessor::groupProcessor(std::string_view group){
  // 先確認目標是否為正確群組
  constexpr std::string_view targetHead = "{\\field";
  if(!start_with(group,targetHead)){
    return std::nullopt;
  }

  // 找到判斷用的關鍵群組
  constexpr std::string_view targetKind = "{\\*\\fldinst";
  size_t pos = group.find(targetKind);
  if(pos == std::string_view::npos){
    return std::nullopt;
  } 
  size_t end = findGroupEnd(group,pos);
  if(end == std::string_view::npos || end <= pos ){
    return std::nullopt;
  }
  std::string_view fldinstGroup = group.substr(pos,end - pos);
  
  // 判斷關鍵群組
  fieldKind key = judgeGroup(fldinstGroup);

  // 依據群組關鍵字決定如何處理
  switch(key){
    case fieldKind::Hyperlink:
    case fieldKind::Page:
    case fieldKind::Ref:
    case fieldKind::Unknown:
      return processFldResult(group);
    case fieldKind::Shape:
      return std::nullopt;
  }

  return std::nullopt;
}

bool fieldGroupProcessor::start_with(std::string_view s,std::string_view prefix){
  return s.size() >= prefix.size() &&
         s.compare(0,prefix.size(),prefix) == 0;
}

size_t fieldGroupProcessor::findGroupEnd(std::string_view group,size_t pos){
  
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

fieldKind fieldGroupProcessor::judgeGroup(std::string_view group){
  constexpr std::string_view marker = "\\fldinst";
  
  size_t pos = group.find(marker);
  if(pos == std::string_view::npos){
    return fieldKind::Unknown;
  }
  pos += marker.size();
  
  while(pos < group.size() &&
        std::isspace(static_cast<unsigned char>(group[pos])))
  {
    pos++;
  }
  
  size_t start = pos;
  while(pos < group.size() &&
        std::isalpha(static_cast<unsigned char>(group[pos])))
  {
    pos++;
  }

  if(start == pos){
    return fieldKind::Unknown;
  }
  
  std::string_view command{group.substr(start,pos - start)};
  
  if(command == "SHAPE"){
    return fieldKind::Shape;
  }
  else if(command == "HYPERLINK"){
    return fieldKind::Hyperlink;
  }
  else if(command == "PAGE"){
    return fieldKind::Page;
  }
  else if(command == "REF"){
    return fieldKind::Ref;
  }
  
  return fieldKind::Unknown;
}
// 共用的{\fldrslt}　群組處理函式 , 有特別處裡需求需再呼叫此函式前執行
std::optional<std::string> fieldGroupProcessor::processFldResult(std::string_view group){
  // 找到目標群組並放入容器中
  std::vector<FieldResultGroup> container = loadingContainer(group);
   
  if(container.empty()){
    return std::nullopt;
  }

  // 處裡容器
  processContainer(group,container);
  
  std::string result;
  result.reserve(group.size());
  
  for(const auto& p:container){
    if(!p.completedStr.empty()){
      result.append(p.completedStr);
    }
  }

  if(result.empty()){
    return std::nullopt;
  }

  return result;
  
}
// 找出 {\fldrslt}　群組並放入容器中　
std::vector<FieldResultGroup> fieldGroupProcessor::loadingContainer(std::string_view group){
  std::vector<FieldResultGroup> result;
  constexpr std::string_view head = "{\\fldrslt";
  size_t pos = group.find(head);
  
  while(pos != std::string_view::npos){
    
    size_t end = findGroupEnd(group,pos);
    if(end == std::string_view::npos || end <= pos ){
       pos++;
       pos = group.find(head,pos);
       continue;
    }
    result.push_back(FieldResultGroup{
        pos,
        end
    });
    
    pos = group.find(head,end);
  }
  
  return result;
}
// 用來處理 {\fldrslt}　群組容器
void fieldGroupProcessor::processContainer(std::string_view source , 
                                           std::vector<FieldResultGroup>& container)
{
  constexpr std::string_view targetHead = "{\\fldrslt";
  
  const  std::array<std::string_view,2> targets = {
    "{\\shp",
    "{\\sp" 
  };


  for(FieldResultGroup&  resultgroup: container){
    size_t start = resultgroup.start;
    size_t end   = resultgroup.end;
    
    if(start >= source.size() || end > source.size() || end <= start){
      continue;
    }
    
    std::string_view fldrslt{source.substr(start,end - start)};
    size_t pos = 0;
    // 二次確認
    if(start_with(fldrslt,targetHead)){
      pos  = targetHead.size();

      if(pos < fldrslt.size() && fldrslt[pos] == ' '){
        ++pos;
      }
    }

    resultgroup.completedStr.reserve(fldrslt.size());
    
    while(pos < fldrslt.size()){
      bool skipped = false;

      if(fldrslt[pos] == '{' || fldrslt[pos] == '}'){
        pos++;
        continue;
      }

      for(size_t i =0;i<targets.size();i++){
        if(pos + targets[i].size() <= fldrslt.size() &&
           fldrslt.compare(pos,targets[i].size(),targets[i]) == 0)
        {
          size_t end = findGroupEnd(fldrslt,pos);
          
          if(end != std::string_view::npos){
            pos = end;
          }else{
            pos++;
          }

          skipped = true;
          break;
        }
      }

      if(skipped){
        continue;
      }

      // 控制符處理
      if(fldrslt[pos] == '\\'){
        if(pos + 1 < fldrslt.size() &&
           (fldrslt[pos + 1] == '\\' ||
            fldrslt[pos + 1] == '{' ||
            fldrslt[pos + 1] == '}'))
        {
          resultgroup.completedStr.push_back(fldrslt[pos + 1]);
          pos += 2;
          continue;
        }

        size_t ctrlStart = pos;
        size_t j = pos + 1; 
        while(j < fldrslt.size() &&
              std::isalpha(static_cast<unsigned char>(fldrslt[j])))
        {
          j++;
        }

        std::string_view ctrl = fldrslt.substr(ctrlStart,j - ctrlStart);

        if(j < fldrslt.size() && fldrslt[j] == '-') j++;
        
        while(j < fldrslt.size() &&
              std::isdigit(static_cast<unsigned char>(fldrslt[j])))
        {
          ++j;
        }

        if(j < fldrslt.size() && fldrslt[j] == ' ') ++j;

        if(ctrl == "\\par" || ctrl == "\\line"){
          resultgroup.completedStr.push_back('\n');
        }
        else if(ctrl == "\\tab"){
          resultgroup.completedStr.push_back('\t');
        }

        pos = j;
        continue;
      }
      
      // 其餘的判斷成正文
      resultgroup.completedStr.push_back(fldrslt[pos]);
      pos++;
    }
  }
}

