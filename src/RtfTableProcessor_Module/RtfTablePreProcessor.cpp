// =====================================================
// Module : RtfTablePreProcessor (implementation)
// =====================================================

#include "RtfTablePreProcessor.h"
#include "RtfGroupProcessor_Module/FieldGroupProcessor.h"
#include "RtfGroupProcessor_Module/StartDestinationProcessor.h"
#include <string>
#include <string_view>
#include <vector>
#include <cstddef>
#include <cctype>
#include <optional>
#include <array>
#include <iostream>
#include "LogSystem_Module/LogSystem.h"

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
    // 全域都會用到的控制符
    Par,
    Line,
    
    // 固定表格控制符
    Cell,
    Row,
    Clmgf,
    Clmrg,
    LastRow,
    TrHdr,
    NestCell,
    NestRow,
    Clvmgf,
    Clvmrg,
    Trowd,
    Intbl,

    // 帶有數字的表格控制符
    Cellx,
    Itap,
    Trleft,
    Trrh,

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
  // 新增名單後要在 void sheetProcessor::processor(std::string& rtfContent) 裡使用才會生效
  constexpr std::array<TableCtrl,14> tableCtrls{
    {
      {"cell", TableCtrlKind::Cell, "@@RTF_TABLE_CELL@@"},
      {"row",  TableCtrlKind::Row , "@@RTF_TABLE_ROW@@"},
      {"clmgf", TableCtrlKind::Clmgf, "@@RTF_TABLE_CLMGF@@"},
      {"clmrg", TableCtrlKind::Clmrg, "@@RTF_TABLE_CLMRG@@"},
      {"lastrow", TableCtrlKind::LastRow, "@@RTF_TABLE_LASTROW@@"},
      {"trhdr",   TableCtrlKind::TrHdr,   "@@RTF_TABLE_TRHDR@@"},
      {"nestcell", TableCtrlKind::NestCell, "@@RTF_TABLE_NESTCELL@@"},
      {"nestrow",  TableCtrlKind::NestRow,  "@@RTF_TABLE_NESTROW@@"},
      {"clvmgf" ,  TableCtrlKind::Clvmgf, "@@RTF_TABLE_CLVMGF@@"},
      {"clvmrg" ,  TableCtrlKind::Clvmrg, "@@RTF_TABLE_CLVMRG@@"},
      {"par"   ,   TableCtrlKind::Par   , "@@RTF_TABLE_PAR@@"},
      {"line"  ,   TableCtrlKind::Line  , "@@RTF_TABLE_LINE@@"},
      {"trowd" ,   TableCtrlKind::Trowd , "@@RTF_TABLE_TROWD@@"},
      {"intbl" ,   TableCtrlKind::Intbl , "@@RTF_TABLE_INTBL@@"}
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
  // 新增名單後要在 void sheetProcessor::processor(std::string& rtfContent) 裡使用才會生效
  constexpr std::array<NumericTableCtrl,4> numericTableCtrls{
    {
      {"cellx", TableCtrlKind::Cellx, "@@RTF_TABLE_CELLX:", "@@"},
      {"itap" , TableCtrlKind::Itap , "@@RTF_TABLE_ITAP:" , "@@"},
      {"trleft",TableCtrlKind::Trleft,"@@RTF_TABLE_TRLEFT:","@@"},
      {"trrh" , TableCtrlKind::Trrh , "@@RTF_TABLE_TRRH:"  ,"@@"}
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

        if(end < s.size() && s[end] == '-'){
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

  // 圖片標記符使用的結構
  struct ImageMarkerMatch{
    std::string marker;
    size_t end;
  };

  // 判斷並回圖片標記符傳結構用的函式
  std::optional<ImageMarkerMatch> matchImageMarker(std::string_view s, size_t pos){
    constexpr std::string_view imgHead    = "@@RTF_IMAGE_";
    constexpr std::string_view imgTail = "@@";

    if(pos + imgHead.size() > s.size() ||
       s.compare(pos, imgHead.size(), imgHead) != 0)
    {
      return std::nullopt;
    }

    size_t imgEnd = s.find(imgTail, pos + imgHead.size());

    if(imgEnd == std::string_view::npos){
      return std::nullopt;
    }

    imgEnd += imgTail.size();

    return ImageMarkerMatch{
      std::string(s.substr(pos, imgEnd - pos)),
      imgEnd
    };
  }

  std::optional<std::string>
  extractPreservedContent(std::string_view group){
    std::string result;
    
    for(size_t i = 0;i < group.size();i++){
      if(auto m = matchImageMarker(group, i)){
        result.append(m -> marker);
        i = m -> end - 1;
      }
    }

    if(!result.empty()){
      return result;
    }

    return std::nullopt;
  }
  
}

void sheetProcessor::processor(std::string& rtfContent,logSystem& logger){
  std::vector<sheetScope> pending;
  std::vector<size_t> groupStack;
  groupStack.reserve(64);

  std::string debugStr;

  rtfContent = preprocessNestedTableGroups(rtfContent);

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
    // 2026/08/18 因為副作用過大拿掉此機制觀察沒有影響後進行移除
    /*
    if(groupStack.size() >= 2){
      sheetscope.sheetstart_ = groupStack.back();
      defaultEnd = findGroupEnd(rtfContent,groupStack.back());
      startedFromGroup = (defaultEnd != std::string_view::npos);
    }else{
      sheetscope.sheetstart_ = i;
    }
    */
    sheetscope.sheetstart_ = i;
   
    sheetscope.newsheetStr_.append(match -> marker);

    bool foundRow = false;

    for(size_t j = i + match->targetSize; j < rtfContent.size();j++){
      if(auto m = matchTableControl(rtfContent,j)){
        // 要在此給定 enum 才會加入標記符
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
          case TableCtrlKind::Clvmgf:
          case TableCtrlKind::Clvmrg:
          case TableCtrlKind::Par:
          case TableCtrlKind::Line:
          case TableCtrlKind::Trrh:
          case TableCtrlKind::Trowd:
          case TableCtrlKind::Intbl:
          sheetscope.newsheetStr_.append(m->marker);
          j = (m->end - 1); // 迴圈會自動在+1
          continue;
          
          case TableCtrlKind::Row :
          {
            sheetscope.newsheetStr_.append(m->marker);
            size_t rowEnd = m->end;

            /*
            if(startedFromGroup &&
              defaultEnd != std::string_view::npos &&
              defaultEnd > rowEnd)
            {
              sheetscope.sheetend_ = defaultEnd;
            }else{
              sheetscope.sheetend_ = rowEnd;
            }
            */

            sheetscope.sheetend_ = rowEnd;

            foundRow = true;
            break;
          }

          default: break;
         
        }
      }

      // 辨別並保留圖片標記符
      if(auto m = matchImageMarker(rtfContent, j)){
        sheetscope.newsheetStr_.append(m->marker);

        j = (m->end - 1);
        continue;
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
            if(auto preserved = extractPreservedContent(groupView)){
              sheetscope.newsheetStr_.append(*preserved);
            }
            
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
          TableToken token = groupProcessor(groupView,logger);
          
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
        else{ // 一般括號包住的情況
          size_t k = findGroupEnd(rtfContent, j);

          if(k == std::string_view::npos){
            break;
          }

          std::string_view groupView{
            rtfContent.data() + j,
            k - j
          };

          TableToken token = groupProcessor(groupView, logger);

          if(!token.text.empty()){
            sheetscope.newsheetStr_.append(token.text);
          }

          j = k - 1;
        }
      }

      if(isNestTablePropsStart(rtfContent,j)){
        if(auto props = processNestTableProps(rtfContent,j)){
          sheetscope.newsheetStr_ += props->text;
          j = props->nextPos - 1;
          continue;
        }
      }

      
      // 吃掉並跳過控制符
      if(c == '\\'){

        
        if(j + 1 < rtfContent.size() &&
           (rtfContent[j + 1] == '{' ||
            rtfContent[j + 1] == '}' ||
            rtfContent[j + 1] == '\\') )
        {
          sheetscope.newsheetStr_ += rtfContent[j + 1];
          j++;
          continue; // 迴圈會自動 +1 所以此處只 +1
        }

        // 處理 \'XX，例如 \'a4\'a4
        if (j + 3 < rtfContent.size() &&
            rtfContent[j + 1] == '\'' &&
            std::isxdigit(static_cast<unsigned char>(rtfContent[j + 2])) &&
            std::isxdigit(static_cast<unsigned char>(rtfContent[j + 3])))
        {
          sheetscope.newsheetStr_.push_back('\\');
          sheetscope.newsheetStr_.push_back('\'');
          sheetscope.newsheetStr_.push_back(rtfContent[j + 2]);
          sheetscope.newsheetStr_.push_back(rtfContent[j + 3]);

          j += 3;
          continue;
        }

        // 保留關鍵 \ucX 控制符
        if(j + 3 < rtfContent.size() && 
           rtfContent[j + 1] == 'u'  &&
           rtfContent[j + 2] == 'c'  &&
           std::isdigit(static_cast<unsigned char>(rtfContent[j+3])))
        {
          size_t k = j+3;
          
          sheetscope.newsheetStr_.push_back('\\');
          sheetscope.newsheetStr_.push_back('u');
          sheetscope.newsheetStr_.push_back('c');
          
          while (k < rtfContent.size() &&
                std::isdigit(static_cast<unsigned char>(rtfContent[k])))
          {
            sheetscope.newsheetStr_.push_back(rtfContent[k]);
            ++k;
          }

          j = k - 1; // 迴圈自己 +1
          continue;
        }

        // 處裡 \uXXXX utf 標記符
        if(j + 1 < rtfContent.size() && rtfContent[j + 1] == 'u'){
          size_t k = j + 2;

          //可能有負號
          if(k < rtfContent.size() && rtfContent[k] == '-'){
            k++;
          }

          size_t digitStart = k;
          while(k < rtfContent.size() &&
                std::isdigit(static_cast<unsigned char>(rtfContent[k])))
          {
            k++;
          }

          if( k > digitStart){
            sheetscope.newsheetStr_.append(rtfContent.substr(j,k-j));
            j = k - 1;
            continue;
          }
        }
        
        
        size_t k = j + 1 ;
     
        while(k < rtfContent.size()&& std::isalpha(static_cast<unsigned char>(rtfContent[k]))){
          k++;
        }

        if (k < rtfContent.size() &&
           (rtfContent[k] == '-' || std::isdigit(static_cast<unsigned char>(rtfContent[k]))))
        {
          if (rtfContent[k] == '-') {
            ++k;
          }

          while (k < rtfContent.size() &&
            std::isdigit(static_cast<unsigned char>(rtfContent[k]))) 
          {
            ++k;
          }
        }

        if (k <rtfContent.size() && rtfContent[k] == ' ') {
          j = k;
          continue;
        }

        // 目前 j 已經在下一個非控制字字元，要先 -1 避免迴圈多跳
        j = k -1 ;
        continue;
      }
      

      // 吃掉單獨的大括號
      if(c == '{' || c == '}'){
        continue;
      }
      
      // 處裡到此步假定為正文將其加入字串中
      // RTF 原始格式中的換行不屬於正文內容
      if(c != '\n' && c != '\r'){
        sheetscope.newsheetStr_.push_back(c);
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
TableToken sheetProcessor::groupProcessor(std::string_view groupView,logSystem& logger){
  // 先移除一般群組內部內的 shp 群組
  std::string cleanedGroup = removeTableShapes(groupView);

  //轉移指向的目標
  groupView = cleanedGroup;
  
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
        case TableCtrlKind::Clvmgf:
        case TableCtrlKind::Clvmrg:
        case TableCtrlKind::Par:
        case TableCtrlKind::Line:
        case TableCtrlKind::Trrh:
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

    if(c == '{'){
      size_t k = findGroupEnd(groupView, i);

      if(k != std::string_view::npos){
        std::string_view nested = groupView.substr(i, k - i);

        if(start_with(nested, "{\\field")){
          fieldGroupProcessor processor;

          if(auto text = processor.groupProcessor(nested)){
            token.text += *text;
          }

          i = k - 1;
          continue;
        }

        if(start_with(nested, "{\\*\\")){
          // 星號特別群組交由專用模組處裡
          StartGroupProcessor startProcessor;
          StarGroupResult sg = startProcessor.groupProcessor(nested);

          // 名單內的就全部移除
          if(sg.unknown == false){
            i = k - 1;
            continue;
          }else{
            if(!sg.unknownGroupName.empty()){
              logger.log(
                LogLevel::Warn,
                "Unknown star group: " +
                sg.unknownGroupName
              );
            }

            if(sg.text){
              token.text += *sg.text;
            }

            i = k - 1;
            continue;
          }
        }
      }

      continue;
    }

    if(c == '}') continue;

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
// 負責在流程處理前預先處理巢狀迴圈相關群組
std::string sheetProcessor::preprocessNestedTableGroups(std::string_view group){
  std::string result;
  result.reserve(group.size());

  // 替換目標的巢狀群組開頭
  constexpr std::string_view nonestHead = "{\\nonesttables";
  constexpr std::string_view nestHead   = "{\\*\\nesttableprops";

  // 巢狀群組替換成的標記符名稱
  constexpr std::string_view nestPropsStartMarker = "@@RTF_NEST_TABLE_PROPS_START@@";
  constexpr std::string_view nestPropsEndMarker = "@@RTF_NEST_TABLE_PROPS_END@@";

  for(size_t i =0;i<group.size();){
    
    if(i + nonestHead.size() <= group.size() &&
       group.compare(i,nonestHead.size(),nonestHead) == 0 &&
       isControlEnd(group,i + nonestHead.size()))
    {
      const size_t end = findGroupEnd(group,i);

      // 群組不完整依舊繼續執行
      if(end == std::string_view::npos){
        result.push_back(group[i]);
        i++;
        continue;
      }

      i = end;
      continue;
    }

    if(i + nestHead.size() <= group.size() &&
       group.compare(i,nestHead.size(),nestHead) == 0 &&
       isControlEnd(group,i + nestHead.size()))
    {
      const size_t end = findGroupEnd(group, i);

      if(end == std::string_view::npos){
        result.push_back(group[i]);
        i++;
        continue;
      }

      size_t contentBegin = i + nestHead.size();

      // 跳過控制字後可選的單一 delimiter space。
      if(contentBegin < end && group[contentBegin] == ' '){
        ++contentBegin;
      }

      // end 指向 '}' 後一格，因此 end - 1 是外層結尾 '}'
      const size_t contentEnd = end - 1;

      result.append(nestPropsStartMarker);

      if(contentBegin < contentEnd){
        result.append(group.substr(contentBegin,contentEnd - contentBegin));
      }

      result.append(nestPropsEndMarker);
     
      i = end;
      continue;
    }
    
    result.push_back(group[i]);
    ++i;
  }

  return result;
}

bool sheetProcessor::isNestTablePropsStart(std::string_view group,size_t pos){
  
  constexpr std::string_view nestPropsStartMarker = "@@RTF_NEST_TABLE_PROPS_START@@";
  
  return pos + nestPropsStartMarker.size() <= group.size() &&
         group.compare(pos,nestPropsStartMarker.size(),nestPropsStartMarker) == 0;
}

std::optional<NestTablePropsResult> 
sheetProcessor::processNestTableProps(std::string_view group,size_t pos){
  constexpr std::string_view nestPropsStartMarker = "@@RTF_NEST_TABLE_PROPS_START@@";
  constexpr std::string_view nestPropsEndMarker = "@@RTF_NEST_TABLE_PROPS_END@@";

  std::string result;
  result.reserve(group.size());

  auto skipControlDelimiter = [](std::string_view s, size_t& i) {
    while (i < s.size() &&
           (s[i] == ' ' || s[i] == '\r' || s[i] == '\n')) {
        ++i;
    }
  };

  size_t i = pos;
  if(i + nestPropsStartMarker.size() <= group.size() &&
     group.compare(i,nestPropsStartMarker.size(),nestPropsStartMarker) == 0)
  {
    result.append(nestPropsStartMarker);
    i += nestPropsStartMarker.size();
  }
  else{
    return std::nullopt;
  }

  for(;i <group.size();){
    
    if(i + nestPropsEndMarker.size() <= group.size() &&
       group.compare(i,nestPropsEndMarker.size(),nestPropsEndMarker) == 0)
    {
      result.append(nestPropsEndMarker);
      i += nestPropsEndMarker.size();
     
      return NestTablePropsResult{std::move(result),i};
    }

     if(auto m = matchTableControl(group,i)){
      switch(m->kind){
        case TableCtrlKind::Cell:
        case TableCtrlKind::Row:
        case TableCtrlKind::Cellx :
        case TableCtrlKind::Clmgf :
        case TableCtrlKind::Clmrg :
        case TableCtrlKind::LastRow:
        case TableCtrlKind::TrHdr:
        case TableCtrlKind::NestCell:
        case TableCtrlKind::NestRow:
        case TableCtrlKind::Itap:
        case TableCtrlKind::Trleft:
        case TableCtrlKind::Clvmgf:
        case TableCtrlKind::Clvmrg:
        case TableCtrlKind::Par:
        case TableCtrlKind::Line:
        case TableCtrlKind::Trrh:
        case TableCtrlKind::Trowd:
        case TableCtrlKind::Intbl:
        result.append(m->marker);
        i = m->end;
        skipControlDelimiter(group,i);//吃掉關鍵控制符後方的空格
        continue;

        default :
        break;
      }  
    }
    
    i++;
  }

  return std::nullopt;
}

// 檢查並處裡找到群組中的 shp 群組
std::string sheetProcessor::removeTableShapes(std::string_view group){
  constexpr std::string_view  target = "{\\shp";
  
  if(group.find(target) == std::string::npos){
    return std::string(group);
  } 

  std::string result;
  result.reserve(group.size());

  for(size_t i =0;i<group.size();){

    if(i + target.size() <= group.size() &&
       group.compare(i,target.size(),target) == 0 &&
       isControlEnd(group,i + target.size()))
    {
      size_t j = findGroupEnd(group,i);

      if(j == std::string::npos){
        return std::string(group);
      }

      // 目前採取 : 整個圖形群組跳過的策略
      i = j;
      continue;
    }

    result.push_back(group[i]);
    i++;
  }

  return result;
}