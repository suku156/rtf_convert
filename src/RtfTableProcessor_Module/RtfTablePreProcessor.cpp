// =====================================================
// Module : RtfTablePreProcessor (implementation)
// =====================================================

#include "RtfTablePreProcessor.h"
#include "RtfGroupProcessor_Module/FieldGroupProcessor.h"
#include <string>
#include <string_view>
#include <vector>
#include <cstddef>
#include <cctype>
#include <optional>
#include <array>

// 與表格先關的控制符名單與轉換成的標記符相關
// 固定資訊控制符: \cell \row.... 等不含數字資訊之控制符
// 非固定資訊控制符 : \cellxN.... 等含有數字資訊之控制符
namespace{
  
  bool isControlEnd(std::string_view s,size_t pos){
    return pos >= s.size() ||
           !std::isalpha(static_cast<unsigned char>(s[pos]));
  }
  // 用於辨別如何處裡的共用 enum (固定/非固定 控制符共用)
  enum class TableCtrlKind{
    Cell,
    Row,
    Clmgf,
    Clmrg,
    LastRow,
    TrHdr,
    NestCell,
    NestRow,

    Cellx,
    Itap,
    Trleft,

    Unknown
  };
  // 規格表結構 (固定資訊控制符使用)
  struct TableCtrl{
    // 目標控制符名稱
    std::string_view word; 
    // 對應之 enum
    TableCtrlKind kind;
    // 控制符對應之標記符
    std::string_view marker;
  };
  // 主要使用的規格表 (固定資訊控制符使用)
  constexpr std::array<TableCtrl,8> tableCtrls{
    {
      {"cell", TableCtrlKind::Cell, "@@RTF_TABLE_CELL@@"},
      {"row",  TableCtrlKind::Row , "@@RTF_TABLE_ROW@@"},
      {"clmgf", TableCtrlKind::Clmgf, "@@RTF_TABLE_CLMGF@@"},
      {"clmrg", TableCtrlKind::Clmrg, "@@RTF_TABLE_CLMRG@@"},
      {"lastrow", TableCtrlKind::LastRow, "@@RTF_TABLE_LASTROW@@"},
      {"trhdr",   TableCtrlKind::TrHdr,   "@@RTF_TABLE_TRHDR@@"},
      {"nestcell", TableCtrlKind::NestCell, "@@RTF_TABLE_NESTCELL@@"},
      {"nestrow",  TableCtrlKind::NestRow,  "@@RTF_TABLE_NESTROW@@"}
    }
  };
  // 規格表結構 (非固定資訊控制符使用)
  struct NumericTableCtrl {
    // 目標控制符名稱
    std::string_view word;
    // 對應之 cnum
    TableCtrlKind kind;
    // 對應標記符之開頭
    std::string_view markerPrefix;
    // 對應標記符之結尾
    std::string_view markerSuffix;
  };
  // 主要使用的規格表 (非固定資訊控制符使用)
  constexpr std::array<NumericTableCtrl,3> numericTableCtrls{
    {
      {"cellx", TableCtrlKind::Cellx, "@@RTF_TABLE_CELLX:", "@@"},
      {"itap" , TableCtrlKind::Itap , "@@RTF_TABLE_ITAP:" , "@@"},
      {"trleft",TableCtrlKind::Trleft,"@@RTF_TABLE_TRLEFT:","@@"}
    }
  };
  // 辨別函式使用之回傳結構 (固定/非固定 控制符共用)
  struct TableCtrlMatch{
    TableCtrlKind kind = TableCtrlKind::Unknown;

    std::string_view word;
    std::string marker;
    
    size_t end;
  };
  // 固定控制符判斷函式
  std::optional<TableCtrlMatch> matchFixedTableControl(std::string_view s, size_t pos){
    if(pos >= s.size() || s[pos] != '\\'){
      return std::nullopt;
    }

    for(const TableCtrl& ctrl : tableCtrls){
      size_t len = ctrl.word.size();
      size_t end = pos + 1 + len;

      if(end <= s.size() &&
         s.compare(pos +1,len,ctrl.word) == 0 &&
         isControlEnd(s,end))
      {
        return TableCtrlMatch{
          ctrl.kind,
          ctrl.word,
          std::string{ctrl.marker},
          end
        };
      }
    }

    return std::nullopt;
  }
  // 非固定控制符判斷函式
  std::optional<TableCtrlMatch> matchNumericTableControl(std::string_view s, size_t pos){
    if(pos >= s.size() || s[pos] != '\\'){
      return std::nullopt;
    }

    for(const NumericTableCtrl& numCtrl : numericTableCtrls){
      size_t len = numCtrl.word.size();
      size_t nameEnd = pos + 1 + len;
      
      if(nameEnd <= s.size() &&
         s.compare(pos +1,len,numCtrl.word) == 0)
      {
        size_t numStart = nameEnd;
        size_t end = numStart;

        if(numCtrl.kind == TableCtrlKind::Trleft &&
           end < s.size() && 
           s[end] == '-'){
          end++;
        }
        
        size_t digitStart = end;
      
        while(end < s.size() &&
              std::isdigit(static_cast<unsigned char>(s[end])))
        {
          end++;
        }

        if(end == digitStart){
          return std::nullopt;
        }

        if(!isControlEnd(s,end)){
          return std::nullopt;
        }

        std::string_view numStr{s.substr(numStart,end - numStart)};
        
        std::string marker;
        marker.reserve(numCtrl.markerPrefix.size() + 
                       numStr.size() + 
                       numCtrl.markerSuffix.size());
        
        marker.append(numCtrl.markerPrefix);
        marker.append(numStr);
        marker.append(numCtrl.markerSuffix);

        return TableCtrlMatch{
          numCtrl.kind,
          numCtrl.word,
          marker,
          end
        };
      }
    }

    return std::nullopt;
  }
  // 判斷並回傳結構用的函式
  std::optional<TableCtrlMatch> matchTableControl(std::string_view s,size_t pos){
    if (auto m = matchFixedTableControl(s, pos)) {
      return m;
    }

    if(auto m = matchNumericTableControl(s,pos)){
      return m;
    }

    return std::nullopt;
  }
}

void sheetProcessor::processor(std::string& rtfContent){
  std::vector<sheetScope> pending;
  std::vector<size_t> groupStack;
  groupStack.reserve(64);

  for(size_t i = 0 ; i < rtfContent.size();i++){
    char c = rtfContent[i];

    // 跳過 escaped \{ \} 
    if(c == '\\' && i + 1 < rtfContent.size() &&
       (rtfContent[i + 1] == '{' ||
        rtfContent[i + 1] == '}' ||
        rtfContent[i + 1] == '\\'))
    {
      ++i;
      continue;
    }

    if(c == '{'){
      groupStack.push_back(i);
      continue;
    }
    
    if(c == '}'){
      if(!groupStack.empty()){
        groupStack.pop_back();
      }
      continue;
    }

    std::optional<TableStartMatch> match = matchTableStart(rtfContent,i);
    if(!match){
      continue;
    }

    sheetScope sheetscope;
    size_t defaultEnd = std::string_view::npos;
    bool startedFromGroup = false;

    // 藉由 {} 位置之紀錄決定開頭之正確位置
    if(groupStack.size() >= 2){
      sheetscope.sheetstart_ = groupStack.back();
      defaultEnd = findGroupEnd(rtfContent,groupStack.back());
      startedFromGroup = (defaultEnd != std::string_view::npos);
    }else{
      sheetscope.sheetstart_ = i;
    }
   
    sheetscope.newsheetStr_.append(match -> marker);

    bool foundRow = false;

    for(size_t j = i + match->targetSize; j < rtfContent.size();j++){
      if(auto m = matchTableControl(rtfContent,j)){
        switch(m->kind){
          case TableCtrlKind::Cell:
          case TableCtrlKind::Cellx :
          case TableCtrlKind::Clmgf :
          case TableCtrlKind::Clmrg :
          case TableCtrlKind::LastRow :
          case TableCtrlKind::TrHdr :
          case TableCtrlKind::NestCell:
          case TableCtrlKind::NestRow:
          case TableCtrlKind::Itap:
          case TableCtrlKind::Trleft:
          sheetscope.newsheetStr_.append(m->marker);
          j = (m->end - 1); // 迴圈會自動在+1
          continue;
          
          case TableCtrlKind::Row :
          {
            sheetscope.newsheetStr_.append(m->marker);
            size_t rowEnd = m->end;

            if(startedFromGroup &&
              defaultEnd != std::string_view::npos &&
              defaultEnd > rowEnd)
            {
              sheetscope.sheetend_ = defaultEnd;
            }else{
              sheetscope.sheetend_ = rowEnd;
            }
            
            foundRow = true;
            break;
          }

          default: break;
         
        }
      }

      if(foundRow){
        break;
      }

      // 處理再表格範圍內會遇到的群組情況
      char c = rtfContent[j];
      if(c == '{'){
        // 跳過可能存在的空格
        size_t controlPos = j + 1;
        while(controlPos < rtfContent.size() &&
              std::isspace(static_cast<unsigned char>(rtfContent[controlPos])))
        {
          ++controlPos;
        }  
          
        if(controlPos < rtfContent.size() && rtfContent[controlPos] == '\\'){
          
          size_t k = findGroupEnd(rtfContent,j);
          if(k == std::string_view::npos){
            break;
          }

          std::string_view groupView{rtfContent.data()+ j ,k - j};

          // 1. 先攔截 shape 類群組：直接跳過
          if (start_with(groupView, "{\\shp") ||
              start_with(groupView, "{\\*\\shpinst") ||
              start_with(groupView, "{\\*\\shppict") ||
              start_with(groupView, "{\\*\\picprop")) 
          {
            j = k - 1;
            continue;
          }

          // 2. 處裡 field 群組
          if(start_with(groupView, "{\\field")){
            fieldGroupProcessor fieldgroupprocessor;
            
            std::optional<std::string> fieldText = 
                  fieldgroupprocessor.groupProcessor(groupView);
            
            if(fieldText){
              sheetscope.newsheetStr_.append(*fieldText);
            }
            
            j = k - 1;
            continue;
          }

          // 3.處裡一般群組查詢有無正文或關鍵控制符
          bool groupHasRow = false;
          TableToken token = groupProcessor(groupView);
          
          if(!token.text.empty()){
            sheetscope.newsheetStr_.append(token.text);
          }
          
          if(token.hasRow){
            groupHasRow = true;
          }
          
          j = k - 1;

          if(groupHasRow){
            size_t rowEnd = k;
            if(startedFromGroup &&
               defaultEnd != std::string_view::npos &&
               defaultEnd > rowEnd)
            {
              sheetscope.sheetend_ = defaultEnd;
            }else{
              sheetscope.sheetend_ = rowEnd;
            }
            foundRow = true;
            break;
          }

        }
      }
    }

    if(foundRow){
      size_t nextPos = sheetscope.sheetend_;
      
      pending.push_back(std::move(sheetscope));

      buildGroupStackUntil(rtfContent,groupStack,i,nextPos);
     
      i = nextPos - 1;
      continue;
    }else{
      i += match->targetSize - 1;
      continue;
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

std::optional<TableStartMatch> 
sheetProcessor::matchTableStart(std::string_view s, size_t i){
  constexpr std::string_view trowd = "\\trowd";
  constexpr std::string_view intbl = "\\intbl";

  if(i + trowd.size() <= s.size() &&
     s.compare(i, trowd.size(), trowd) == 0)
  {
    return TableStartMatch{trowd.size(), "@@RTF_TABLE_TROWD@@"};
  }
    
  if(i + intbl.size() <= s.size() &&
     s.compare(i, intbl.size(), intbl) == 0)
  {
    return TableStartMatch{intbl.size(), "@@RTF_TABLE_INTBL@@"};
  }

  return std::nullopt;
}

void sheetProcessor::buildGroupStackUntil(std::string_view group,
                                          std::vector<size_t>& groupStack,
                                          size_t start,size_t end)
{
  end = std::min(end, group.size());
  
  for(size_t i = start;i < end;i++){
    char c = group[i];

    // 跳過 escaped \{ \} 
    if(c == '\\' && i + 1 < group.size() &&
       (group[i + 1] == '{' ||
        group[i + 1] == '}' ||
        group[i + 1] == '\\'))
    {
      ++i;
      continue;
    }
    
    if(c == '{'){
      groupStack.push_back(i);
      continue;
    }
    
    if(c == '}'){
      if(!groupStack.empty()){
        groupStack.pop_back();
      }
      continue;
    }
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

  auto skipControlDelimiter = [](std::string_view s, size_t& i) {
    while (i + 1 < s.size() &&
           (s[i + 1] == ' ' || s[i + 1] == '\r' || s[i + 1] == '\n')) {
        ++i;
    }
  };

  for(size_t i = 0;i<groupView.size();i++){
    if(auto m = matchTableControl(groupView,i)){
      switch(m->kind){
        case TableCtrlKind::Cell:
        token.hasCell = true;
        token.type = TableTokenType::cell;
        token.text += m->marker;
        i = m->end -1;
        skipControlDelimiter(groupView,i);//吃掉關鍵控制符後方的空格
        continue;

        case TableCtrlKind::Row:
        token.hasRow = true;
        token.type = TableTokenType::row;
        token.text += m->marker;
        i = m->end -1;
        skipControlDelimiter(groupView,i);//吃掉關鍵控制符後方的空格
        continue;

        case TableCtrlKind::Cellx :
        case TableCtrlKind::Clmgf :
        case TableCtrlKind::Clmrg :
        case TableCtrlKind::LastRow:
        case TableCtrlKind::TrHdr:
        case TableCtrlKind::NestCell:
        case TableCtrlKind::NestRow:
        case TableCtrlKind::Itap:
        case TableCtrlKind::Trleft:
        token.text += m->marker;
        i = m->end -1;
        skipControlDelimiter(groupView,i);//吃掉關鍵控制符後方的空格
        continue;

        default :
        break;
      }  
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

bool sheetProcessor::start_with(std::string_view s,std::string_view prefix){
  return s.size() >= prefix.size() &&
         s.compare(0,prefix.size(),prefix) == 0;
}

size_t sheetProcessor::findGroupEnd(std::string_view group,size_t pos){
  
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