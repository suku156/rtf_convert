// =====================================================
// Module : TxtTableRenderer (implementation)
// =====================================================

#include "TxtTableRenderer.h"
#include "SemanticStructure_Module/Document.h"
#include<ostream>
#include<string>
#include<cstddef>
#include <utility>
#include <algorithm>
#include <set>
#include <vector>

namespace txt_table{
  struct CellResult {
    std::string text;
    int width = 0;

    // 水平線的左右邊界(共用水平參考線用)
    int leftBoundary = 0;
    int rightBoundary = 0;

    // 垂直合併用數據
    bool verticalMergeStart = false;
    bool verticalMerged = false;

    // 水平合併用數據
    bool horizontalMergeStart = false;
    bool horizontalMerged = false;

    // 只供 renderer 讀取，不負責生命週期
    const TableBlock* nestedTable = nullptr;

    // 在有表格的情況下用來判斷是文字還是表格優先
    NestedTablePlacement nestedTablePlacement = NestedTablePlacement::None;
  };

  struct RowResult {
    std::vector<CellResult> cells;
    
    // 該列左起點：\trleft
    int leftBoundary = 0;
    
    // 紀錄每一列最後一個 cellx , 也就是該列右邊界
    int rightBoundary = 0;
    
    int trrh = 0;
  };

  struct TableResult {
    std::vector<RowResult> rows;
  };  

  std::string parsePictMarker(std::string_view text){
    constexpr std::string_view imgHead = "@@RTF_IMAGE";
    constexpr std::string_view imgErr  = "_ERR_";

    std::string result;
    result.reserve(text.size());

    for(size_t i = 0;i < text.size();i++){

      if(i + imgHead.size() <= text.size() &&
         text.compare(i, imgHead.size(), imgHead) == 0)
      {
        size_t j = i + imgHead.size();

        if(j + imgErr.size() <= text.size() &&
           text.compare(j, imgErr.size(), imgErr) == 0)
        {
          size_t numStart = j + imgErr.size();
          
          size_t MarkerEnd = text.find("@@",numStart);
          
          if(MarkerEnd == std::string::npos){
            i = numStart - 1; // 迴圈會自動加
            continue;
          }

          // 擷取數字部份
          std::string num(text.data() + numStart,MarkerEnd - numStart);

          result += "[IMAGE_ERR :  ";
          result += num ;
          result += "]";
         
          i = MarkerEnd + 1; // 跳過最後兩個 @@ 迴圈會在自動加一所以只加一
          continue;

        }else if( j < text.size() && text[j] == '_'){
          size_t numStart = j + 1;
          
          size_t end = text.find("@@",numStart);
          
          if(end == std::string::npos){
            i = numStart - 1; // 迴圈會自動加
            continue;
          }

          // 擷取數字部份
          std::string num(text.data() + numStart,end - numStart);

          result += "[IMAGE :  ";
          result += num ;
          result += "]";
          

          i = end + 1;
          continue;

        }
      }

      result.push_back(text[i]);
    }

    return result;
  }
  
  TableResult buildResult(const TableBlock& table){
    TableResult tableResult;
    tableResult.rows.reserve(table.rows().size());

    for(const auto& row : table.rows()){
      RowResult rowResult;

      rowResult.trrh = row.trrh();
      rowResult.leftBoundary = row.trLeft();
      
      const auto& cells = row.cells();
      const auto & cellDefs = row.cellDefs();

      if(!cellDefs.empty()){
        rowResult.rightBoundary = cellDefs.back().cellx;
      }else{
        rowResult.rightBoundary = rowResult.leftBoundary;
      }
      
      rowResult.cells.reserve(cells.size());

      int prevCellx = rowResult.leftBoundary;

      for(size_t i = 0;i<cells.size();i++){
        CellResult cellResult;
        
        const auto& cellText = cells[i].text();
        
        if(cellText.find("@@RTF_IMAGE") != std::string::npos){
          cellResult.text = parsePictMarker(cellText);
        }else{
          cellResult.text = cellText;
        }
        
        cellResult.nestedTable = cells[i].nestedTable();
        cellResult.nestedTablePlacement = cells[i].nestedTablePlacement(); 

        cellResult.leftBoundary = prevCellx;

        if(i < cellDefs.size()){
         
          const int  currentCellx = cellDefs[i].cellx;

          cellResult.rightBoundary = currentCellx;

          if(currentCellx >= prevCellx){
            cellResult.width = currentCellx - prevCellx;
          }else{
            cellResult.width = 0;
          }
          
          prevCellx = currentCellx;
          
         
          cellResult.verticalMergeStart   = cellDefs[i].verticalMergeStart;
          cellResult.verticalMerged       = cellDefs[i].verticalMerged;
          cellResult.horizontalMergeStart = cellDefs[i].horizontalMergeStart;
          cellResult.horizontalMerged     = cellDefs[i].horizontalMerged;
        }else{
          cellResult.rightBoundary = prevCellx;
          cellResult.width = 0;
        }

        rowResult.cells.push_back(std::move(cellResult));
      }

      tableResult.rows.push_back(std::move(rowResult));
    }

    return tableResult;
  }
  // 因應中文 urf8 新增的對應機制函式
  // 計算字串的安全裁減數字
  size_t utf8CharBytes(unsigned char c){
    if((c & 0x80) == 0) return 1;
    if((c & 0xE0) == 0xC0) return 2;
    if((c & 0xF0) == 0xE0) return 3;
    if((c & 0xF8) == 0xF0) return 4;
    return 1;
  }
  size_t charDisplayWidth(std::string_view s , size_t pos){
    unsigned char c = static_cast<unsigned char>(s[pos]);

    // ASCII
    if(c < 0x80) return 1;

    // 非 ASCII 都先當作寬字元處裡
    return 2;
  }
  // 計算含有 utf8 中文情況下的真實文字寬度
  size_t displayWidth(std::string_view s){
    size_t width = 0;

    for(size_t i = 0; i < s.size(); ){
      width += charDisplayWidth(s, i);
      i += utf8CharBytes(static_cast<unsigned char>(s[i]));
    }

    return width;
  }
  
  size_t divCeil(size_t a, size_t b){
    return (a + b - 1) / b;
  }
    
  // 依照最寬的右邊界決定比例
  size_t chooseTwipsPerChar(size_t maxRightBoundary){
    constexpr size_t DefaultTwipsPerChar = 100;
    constexpr size_t MaxTxtLineWidth = 120;

    if(maxRightBoundary == 0){
        return DefaultTwipsPerChar;
    }

    size_t adjusted = divCeil(maxRightBoundary, MaxTxtLineWidth);

    return std::max(DefaultTwipsPerChar, adjusted);
  }
  // 用來計算最長的列的寬度用的函式
  size_t lineWidthNoNewline(std::string_view s){
    if(!s.empty() && s.back() == '\n'){
      s.remove_suffix(1);
    }

    return displayWidth(s);
  }
  // 用來補齊邊框線的函式
  void padBorderLineToWidth(std::string& line, size_t targetWidth){
    if(!line.empty() && line.back() == '\n'){
      line.pop_back();
    }

    size_t width = displayWidth(line);

    // 如果已經超過，第一版可以直接改成滿版邊框
    if(width > targetWidth){
      line.clear();
      line.push_back('+');
      line.append(targetWidth > 2 ? targetWidth - 2 : 1, '-');
      line.push_back('+');
      line.push_back('\n');
      return;
    }

    // 補在最後一個 '+' 前面
    if(width < targetWidth && !line.empty() && line.back() == '+'){
      line.pop_back();
      line.append(targetWidth - width, '-');
      line.push_back('+');
    }

    line.push_back('\n');
  }
  // 用來補齊文字線的函式(文字完整度優先)
  void padContentLineToWidth(std::string& line, size_t targetWidth){
    size_t start = 0;

    while(start < line.size()){
      size_t end = line.find('\n', start);

      if(end == std::string::npos){
        end = line.size();
      }

      std::string currentLine = line.substr(start, end - start);

      size_t width = displayWidth(currentLine);

      // 文字完整度優先：超過就保留原樣，不裁切
      if(width < targetWidth &&
         !currentLine.empty() &&
         currentLine.back() == '|')
      {
        currentLine.pop_back();

        currentLine.append(
          targetWidth - width,
          ' '
        );

        currentLine.push_back('|');
      }

      // 將原本這一行替換成補齊後的內容
      line.replace(
          start,
          end - start,
          currentLine
      );

      // 移動到下一行
      start += currentLine.size();

      if(start < line.size() &&
         line[start] == '\n')
      {
        ++start;
      }
    }

    // 如果原本最後沒有換行，補上
    if(line.empty() || line.back() != '\n'){
      line.push_back('\n');
    }
  }
  
  // 建立每段表格的共同水平 gird
  std::vector<int> getColspanGrid(const TableResult& result){
    std::set<int> boundaries;

    // 遍歷結構找尋並加入 cellx 座標
    for(const auto& row : result.rows){
      // 每列真正的左起點：\trleft
      boundaries.insert(row.leftBoundary);


      for(const auto& cell : row.cells){
        boundaries.insert(cell.rightBoundary);
      }

      // 防呆用
      boundaries.insert(row.rightBoundary);
      
    }

    return std::vector<int>(boundaries.begin(), boundaries.end());
  }

  // 依據共用水平線 與 計算用數字 得到相應的 共用水平線每一格之寬度
  std::vector<size_t> buildGridWidths(const std::vector<int>& grid,size_t twipsPerChar)
  {
    std::vector<size_t> widths;

    if(grid.size() < 2){
      return widths;
    }

    widths.reserve(grid.size() - 1);

    for(size_t i = 1; i < grid.size(); ++i){
      int diff = grid[i] - grid[i - 1];

      if(diff <= 0){
        widths.push_back(1);
        continue;
      }

      size_t txtWidth = std::max<size_t>(1,static_cast<size_t>(diff) / twipsPerChar);

      widths.push_back(txtWidth);
    }

    return widths;
  }

  // 透過比對 共用水平線 與 cell 本身資料 以此獲得該 cell 寬度
  size_t getCellRenderWidth(const CellResult& cell,const std::vector<int>& grid,
                            const std::vector<size_t>& gridWidths)
  {
    auto leftIt = std::find(
      grid.begin(),
      grid.end(),
      cell.leftBoundary
    );

    auto rightIt = std::find(
      grid.begin(),
      grid.end(),
      cell.rightBoundary
    );

    if(leftIt == grid.end() ||
        rightIt == grid.end())
    {
      return 1;
    }

    size_t leftIndex = static_cast<size_t>(std::distance(grid.begin(), leftIt));

    size_t rightIndex =static_cast<size_t>(std::distance(grid.begin(), rightIt));

    if(rightIndex <= leftIndex){
      return 1;
    }

    size_t width = 0;

    for(size_t i = leftIndex;
        i < rightIndex && i < gridWidths.size();
        ++i)
    {
      width += gridWidths[i];
    }

    return std::max<size_t>(1, width);
  }

  // txt 版本用來解讀巢狀迴圈的函式
  std::string renderNestedTableText(const TableBlock& table,size_t indentLevel = 1)
  {
    TableResult result = buildResult(table);

    std::string output;
    std::string indent(indentLevel * 4, ' ');

    for(const RowResult& row : result.rows){
      output += indent;

      bool firstCell = true;

        for(const CellResult& cell : row.cells){
          if(!firstCell){
            output += " | ";
          }

          output += cell.text;

          if(cell.nestedTable){
            output += '\n';
            output += renderNestedTableText(
              *cell.nestedTable,
              indentLevel + 1
            );
          }

          firstCell = false;
        }

      output += '\n';
    }

    return output;
  }

  // 取得 cell 資訊前先使用函式檢查有無巢狀表格資訊
  std::string buildTxtCellContent(const CellResult& cell){
    std::string text = cell.text;

    if(!cell.nestedTable){
        return text;
    }

    std::string nested =
        renderNestedTableText(*cell.nestedTable);

    switch(cell.nestedTablePlacement){
      case NestedTablePlacement::BeforeText:
        if(text.empty()){
          return nested;
        }
        return text + "\n" + nested;
      case NestedTablePlacement::AfterText:
          if(text.empty()){
              return nested;
          }
          return nested + text;
      default:
        if(text.empty()){
          return nested;
        }
        return text + "\n" + nested;
    }
  }

  // 用來判斷是否在 中文區用的配套函式
  uint32_t decodeUtf8(const std::string& text,size_t pos,size_t& charLen)
  {
    unsigned char c = static_cast<unsigned char>(text[pos]);

    if((c & 0x80) == 0){
      charLen = 1;
      return c;
    }

    if((c & 0xE0) == 0xC0){
        charLen = 2;

        return
            ((c & 0x1F) << 6) |
            (static_cast<unsigned char>(text[pos + 1]) & 0x3F);
    }

    if((c & 0xF0) == 0xE0){
        charLen = 3;

        return
            ((c & 0x0F) << 12) |
            ((static_cast<unsigned char>(text[pos + 1]) & 0x3F) << 6) |
            (static_cast<unsigned char>(text[pos + 2]) & 0x3F);
    }

    if((c & 0xF8) == 0xF0){
        charLen = 4;

        return
            ((c & 0x07) << 18) |
            ((static_cast<unsigned char>(text[pos + 1]) & 0x3F) << 12) |
            ((static_cast<unsigned char>(text[pos + 2]) & 0x3F) << 6) |
            (static_cast<unsigned char>(text[pos + 3]) & 0x3F);
    }

    charLen = 1;
    return c;
  }

  // 將目標依照換行符進行分行的動作
  // 有加入計算長度後進行分行的功能
  std::vector<std::string> splitLines(const std::string& text,size_t width){
    std::vector<std::string> result;

    std::string line;
    size_t lineWidth = 0;

    for(size_t i = 0; i < text.size();){
      // 原始換行
      if(text[i] == '\n'){
        result.push_back(line);
        line.clear();
        lineWidth = 0;
        ++i;
        continue;
      }

      // 判斷目前 UTF-8 字元長度
      size_t charLen = 1;

      unsigned char c = static_cast<unsigned char>(text[i]);

      if((c & 0x80) == 0x00){
        charLen = 1;
      }else if((c & 0xE0) == 0xC0){
        charLen = 2;
      }else if((c & 0xF0) == 0xE0){
        charLen = 3;
      }else if((c & 0xF8) == 0xF0){
        charLen = 4;
      }

      std::string utf8Char = text.substr(i, charLen);

      size_t charWidth = displayWidth(utf8Char);

      // 放不下，先換行
      if(lineWidth > 0 &&
         lineWidth + charWidth > width)
      {
        // 判斷是否為 UTF-8 多 byte 字元
        size_t charLen = 0;

        uint32_t cp = decodeUtf8(text, i, charLen);

        bool isAsciiWord = 
        cp <= 0x7F && ((cp >= 'A' && cp <= 'Z') ||
                       (cp >= 'a' && cp <= 'z') ||
                       (cp >= '0' && cp <= '9'));
                                              
        if(isAsciiWord){
          // 非 中文byte內容 嘗試往前找空白
          size_t spacePos = line.find_last_of(' ');

          if(spacePos != std::string::npos &&
             spacePos + 1 < line.size())
          {
            std::string nextLine = line.substr(spacePos + 1);

            line.erase(spacePos);

            result.push_back(line);

            line = nextLine;
            lineWidth = displayWidth(line);

            continue;
          }
        }

        result.push_back(line);

        line.clear();
        lineWidth = 0;
      }

      line.append(utf8Char);
      lineWidth += charWidth;

      i += charLen;
    }
    
    // 最後一行
    result.push_back(line);

    return result;
  }

  std::string buildTxtRow(
    const RowResult& row,
    const std::vector<int>& grid,
    const std::vector<size_t>& gridWidths)
  {
    std::vector<size_t> horizon;
    horizon.reserve(row.cells.size());
    
    std::vector<std::vector<std::string>> allCellLines;

    for(const CellResult& cell : row.cells){
      size_t width =  getCellRenderWidth(cell,grid,gridWidths);

      horizon.push_back(width);

      std::vector<std::string> cellLines = splitLines(cell.text,width);

      allCellLines.push_back(std::move(cellLines));
    }

    size_t vertical_max = 0;

    for(const auto& cellLines : allCellLines){
      if(cellLines.size() > vertical_max){
        vertical_max = cellLines.size();
      }
    }

    std::vector<std::string> lines(vertical_max);

    for(size_t i = 0 ; i < row.cells.size();i++){
      // 從名單中取出對應的邊線
      size_t width = horizon[i];

      // 從名單中取出對應清單
      std::vector<std::string> cellLines = allCellLines[i];

      // 填充完每個 cell 的對應行數的資料
      for(size_t j = 0;j < lines.size();j++){
        lines[j].append("|");

        // 如果該行有內容就將內容移動
        if(j < cellLines.size()){
          const std::string& text = cellLines[j];
          lines[j].append(text);

          // 計算出含中文的實際長度
          size_t used = displayWidth(text);

          // 補上空格到邊界之前
          if(used < width){
            lines[j].append(
              width - used,
              ' '
            );
          }
        }
        else{
          lines[j].append(
            width,
            ' '
          );
        }
      }
    }

    // 將 整個 row 的資訊結合
    // 補上結尾與換行標記

    std::string result;

    for(auto& line : lines){
      line.append("|");
      line.append("\n");
      result.append(line);
    }

    return result;
  }

  void writeResult(const TableResult& result, std::ostream& out){
    const std::vector<RowResult>& rows = result.rows;

    // 取得共用水平線
    std::vector<int> grid = getColspanGrid(result);

    if(grid.size() < 2){
      return;
    }
    
    // 取得最大的右邊界 cellx 資訊
    size_t maxRightBoundary = static_cast<size_t>(grid.back());
    
    // 以此數字換算 twips 到 TXT 顯示寬度
    size_t TwipsPerChar = chooseTwipsPerChar(maxRightBoundary);

    // 依照得到的 txt 顯示寬度數字 換算共用水平線之寬度
    std::vector<size_t> gridWidths = buildGridWidths(grid, TwipsPerChar);
    
    // 第一輪做好每列原本之字串 , 順便找最大寬度之值
    std::vector<std::pair<std::string, std::string>> lines;
    size_t maxLineWidth = 0;
    for(size_t i = 0;i < rows.size();i++){
      const RowResult& row = rows[i];

      // 遍歷cell 結構完成表格字串 
      std::string firstText;
      std::string secondText;
      for(size_t j = 0;j < row.cells.size();j++){
        const CellResult& cell = row.cells[j];

        size_t gapNum =getCellRenderWidth(cell,grid,gridWidths);

        std::string text = buildTxtCellContent(cell);
        size_t textWidth = displayWidth(text);

        size_t renderWidth = std::max(gapNum,textWidth);

        // 第一行
        firstText.append("+");
        firstText.append(renderWidth,'-');
      }

      // 加入結尾符號
      firstText.append("+\n");
      
      // 第二行
      secondText = buildTxtRow(row, grid, gridWidths);

      // 找到最寬的列的值
      maxLineWidth = std::max(maxLineWidth, lineWidthNoNewline(firstText));
      maxLineWidth = std::max(maxLineWidth, lineWidthNoNewline(secondText));

     
      lines.push_back({std::move(firstText),std::move(secondText)}); 
    }

    // 第二輪完成補齊與輸出的動作
    // 先確認最大值
    constexpr size_t MaxTxtBorderWidth = 130;
    size_t targetWidth = std::min(maxLineWidth, MaxTxtBorderWidth);

    for(size_t i = 0;i < lines.size();i++){
      auto& [firstText, secondText] = lines[i];

      padBorderLineToWidth(firstText, targetWidth);
      padContentLineToWidth(secondText, targetWidth);

      out << firstText;
      out << secondText;

      if(i == lines.size() - 1){
        out << firstText;
      }
    }
  }
 
  // 外部呼叫的函式入口
  void renderTable(const TableBlock& table, std::ostream& out){
    auto result = txt_table::buildResult(table);
    txt_table::writeResult(result,out);
  }

  
}