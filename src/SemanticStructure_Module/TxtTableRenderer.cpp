// =====================================================
// Module : TxtTableRenderer (implementation)
// =====================================================

#include "TxtTableRenderer.h"
#include "SemanticStructure_Module/Document.h"
#include<ostream>
#include<string>
#include <utility>

namespace txt_table{

  struct CellResult {
    std::string text;
    size_t width = 0;
  };

  struct RowResult {
    std::vector<CellResult> cells;
    // 紀錄每一列最後一個 cellx , 也就是該列右邊界
    size_t rightBoundary = 0;
  };

  struct TableResult {
    std::vector<RowResult> rows;
  };  
  
  TableResult buildResult(const TableBlock& table){
    TableResult tableResult;
    tableResult.rows.reserve(table.rows().size());

    for(const auto& row : table.rows()){
      RowResult rowResult;

      const auto& cells = row.cells();
      const auto& cellxs = row.cellxs();

      if(!cellxs.empty()){
        rowResult.rightBoundary = cellxs.back();
      }
      
      rowResult.cells.reserve(cells.size());
      int prevCellx = 0;
      
      for(size_t i = 0;i<cells.size();i++){
        CellResult cellResult;
        
        cellResult.text = cells[i].text();

        if(i < cellxs.size()){
          int currentCellx = cellxs[i];
          cellResult.width = currentCellx - prevCellx;
          prevCellx = currentCellx;
        }else{
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
  // 找出右邊界最寬的值
  size_t findMaxRightBoundary(const std::vector<RowResult>& rows){
    size_t maxRightBoundary = 0;

    for(const auto& row : rows){
     maxRightBoundary = std::max(maxRightBoundary,row.rightBoundary);
    }

    return maxRightBoundary;
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
    if(!line.empty() && line.back() == '\n'){
      line.pop_back();
    }

    size_t width = displayWidth(line);

    // 文字完整性優先：超過就保留原樣，不裁切
    if(width < targetWidth && !line.empty() && line.back() == '|'){
      line.pop_back();
      line.append(targetWidth - width, ' ');
      line.push_back('|');
    }

    line.push_back('\n');
  } 

  void writeResult(const TableResult& result, std::ostream& out){
    const std::vector<RowResult>& rows = result.rows;
    
    // 取得最大的右邊界 cellx 資訊
    size_t maxRightBoundary = findMaxRightBoundary(rows);
    
    // 以此數字換算 twips 到 TXT 顯示寬度
    size_t TwipsPerChar = chooseTwipsPerChar(maxRightBoundary);;
    
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

        size_t gapNum = std::max<size_t>(1,cell.width / TwipsPerChar);

        std::string text = cell.text;
        size_t textWidth = displayWidth(text);

        size_t renderWidth = std::max(gapNum,textWidth);

        // 第一行
        firstText.append("+");
        firstText.append(renderWidth,'-');

        // 第二行
        secondText.append("|");
        secondText.append(text);

        if(renderWidth > textWidth){
          secondText.append(renderWidth - textWidth,' ');
        }
      }

      // 加入結尾符號
      firstText.append("+\n");
      secondText.append("|\n");

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

  // utf8中文可用的安全版裁切(目前沒用到,如果有裁切模式的需求在用)
  /*
  std::string cutToDisplayWidth(std::string_view s, size_t maxWidth){
    std::string result;
    size_t width = 0;

    for(size_t i = 0; i < s.size(); ){
      size_t bytes = utf8CharBytes(static_cast<unsigned char>(s[i]));
      size_t charWidth = charDisplayWidth(s, i);

      if(width + charWidth > maxWidth){
        break;
      }

      result.append(s.substr(i, bytes));
      width += charWidth;
      i += bytes;
    }

    return result;
  }
  */
}