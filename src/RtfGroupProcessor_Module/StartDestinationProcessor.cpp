#include "StartDestinationProcessor.h"
#include <string>
#include <string_view>
#include <optional>
#include <vector>
#include <cctype>

// 公開處理函式
StarGroupResult StartGroupProcessor::groupProcessor(std::string_view group){
  StarGroupResult startResult;
  // 簡單防呆
  if(!start_with(group,"{\\*\\")){
    startResult.unknown = true;
    startResult.unknownGroupName = "Not Star Group";
    return startResult;
  }
  // 看看是否是在黑名單中
  if(isKnownDroppableGroup(group)){
    return startResult;
  }
  // 如果是在白名單中就進行清理已找出訊息
  if(isKnownTextCarrierGroup(group)){
    startResult.text = extractVisibleText(group);
    return startResult;
  }

  startResult.unknown = true;
  startResult.unknownGroupName = findStartGroupName(group);

  return startResult;
}

bool StartGroupProcessor::start_with(std::string_view s,std::string_view prefix){
    return s.size() >= prefix.size() &&
           s.compare(0,prefix.size(),prefix) == 0;
}
// 特定可忽略之開頭群組清單
bool StartGroupProcessor::isKnownDroppableGroup(std::string_view g){
    return start_with(g, "{\\*\\generator")     ||
           start_with(g, "{\\*\\userprops")     ||
           start_with(g, "{\\*\\docvar")        ||
           start_with(g, "{\\*\\xmlnstbl")      ||
           start_with(g, "{\\*\\datastore")     ||
           start_with(g, "{\\*\\latentstyles")  ||
           start_with(g, "{\\*\\listtable")     ||
           start_with(g, "{\\*\\listoverridetable") ||
           start_with(g, "{\\*\\rsidtbl")       ||
           start_with(g, "{\\*\\revtbl")        ||
           start_with(g, "{\\*\\pgptbl")        ||
           start_with(g, "{\\*\\colorschememapping") ||
           start_with(g, "{\\*\\themedata")     ||
           start_with(g, "{\\*\\picprop")       ||
           start_with(g, "{\\*\\blipuid")       ||
           start_with(g, "{\\*\\datafield");
}
// 特定需處裡之開頭群組清單
bool StartGroupProcessor::isKnownTextCarrierGroup(std::string_view g){
    return start_with(g, "{\\*\\nesttableprops");
}
// 對特定之群組進行清理
std::string StartGroupProcessor::extractVisibleText(std::string_view g){
  std::string out;
  out.reserve(g.size());

  for (size_t i = 0; i < g.size(); ++i) {
    char c = g[i];

    if (c == '{' || c == '}') {
      continue;
    }

    if (c == '\\') {
      if (i + 1 >= g.size()) break;

      char next = g[i + 1];

      // escaped literal
      if (next == '\\' || next == '{' || next == '}') {
        out.push_back(next);
        ++i;
        continue;
      }

      // control symbol，例如 \~ \- \_
      if (!std::isalpha(static_cast<unsigned char>(next))) {
        ++i;
        continue;
      }

      // control word
      ++i;
      while (i < g.size() && std::isalpha(static_cast<unsigned char>(g[i]))) {
        ++i;
      }

      // optional signed numeric parameter
      if (i < g.size() && (g[i] == '-' || std::isdigit(static_cast<unsigned char>(g[i])))) {
        if (g[i] == '-') ++i;
        while (i < g.size() &&
               std::isdigit(static_cast<unsigned char>(g[i]))) 
        {
          ++i;
        }
      }

      // optional delimiter space
      if (i < g.size() && g[i] == ' ') {
          // 吃掉控制字後的分隔空白
      } else {
          --i;
      }

      continue;
    }

    out.push_back(c);
  }

  return out;
}
// 找出群組名稱方便進行記錄
std::string StartGroupProcessor::findStartGroupName(std::string_view g){
  constexpr std::string_view targetHead = "{\\*\\";
  if(!start_with(g,targetHead)){
    return "Unknown Group Name";
  }
  size_t pos = targetHead.size();
  size_t end = pos;

  while (end < g.size() && std::isalpha(static_cast<unsigned char>(g[end]))) {
    ++end;
  }

  if(end == pos){
    return "Invalid Star Group";
  }
  
  std::string result{g.substr(pos , end - pos)};
  return result;
}