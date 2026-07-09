// =====================================================
// Module : HtmlTableRenderer (implementation)
// =====================================================

#include "HtmlTableRenderer.h"
#include "SemanticStructure_Module/Document.h"
#include<ostream>
#include<string>
#include<cstddef>
#include <utility>
#include<set>
#include<optional>
#include<algorithm>

namespace html_table{
  struct CellResult {
    std::string text;
    size_t width = 0;

    size_t leftBoundary = 0;
    size_t rightBoundary = 0;

    bool verticalMergeStart = false;
    bool verticalMerged = false;
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
      const auto & cellDefs = row.cellDefs();

      if(!cellDefs.empty()){
        rowResult.rightBoundary = cellDefs.back().cellx;
      }
      
      rowResult.cells.reserve(cells.size());
      size_t prevCellx = 0;

      for(size_t i = 0;i<cells.size();i++){
        CellResult cellResult;
        
        
        cellResult.text = cells[i].text();

        cellResult.leftBoundary =prevCellx;

        if(i < cellDefs.size()){
          size_t currentCellx = static_cast<size_t>(cellDefs[i].cellx);

          cellResult.rightBoundary = currentCellx;
          cellResult.width = currentCellx - prevCellx;

          prevCellx = currentCellx;

          cellResult.verticalMergeStart = cellDefs[i].verticalMergeStart;
          cellResult.verticalMerged = cellDefs[i].verticalMerged;
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

  // html 需要的安全符號替換
  void writeHtmlEscaped(std::string_view text, std::ostream& out){
    for(char c : text){
        switch(c){
        case '&': out << "&amp;"; break;
        case '<': out << "&lt;"; break;
        case '>': out << "&gt;"; break;
        case '"': out << "&quot;"; break;
        case '\'': out << "&#39;"; break;
        default: out << c; break;
        }
    }
  }

  // 建立每段表格的共同水平 gird
  std::vector<size_t> getColspanGrid(const TableResult& result){
    std::set<size_t> boundaries;

    // 起點
    boundaries.insert(0);

    // 遍歷結構找尋並加入 cellx 座標
    for(const auto& row : result.rows){
      size_t current = 0;

      for(const auto& cell : row.cells){
        current += cell.width;

        if(current > 0){
          boundaries.insert(current);
        }
      }

      if(row.rightBoundary > 0){
        boundaries.insert(row.rightBoundary);
      }
    }

    return std::vector<size_t>(boundaries.begin(), boundaries.end());
  }
  // 用來查找在共用水平座標列的位置
  std::optional<size_t> findGridIndex(const std::vector<size_t>& grid, size_t value){
    auto it = std::find(grid.begin(), grid.end(), value);

    if(it == grid.end()){
      return std::nullopt;
    }

    return static_cast<size_t>(std::distance(grid.begin(), it));
  }
  // 使用流程中的小工具用來回傳要跨幾格(水平線用)
  size_t getColspan(const std::vector<size_t>& grid,size_t left,size_t right){
    size_t colspan = 1;

    auto leftIndex = findGridIndex(grid, left);
    auto rightIndex = findGridIndex(grid, right);

    if(leftIndex && rightIndex && *rightIndex > *leftIndex){
      colspan = *rightIndex - *leftIndex;
    }
    
    return colspan;
  }
  // 推算要跳過幾格的函式(垂直線用)
  size_t getRowspan(const TableResult& result,
                    size_t rowIndex,
                    size_t left,
                    size_t right)
  {
    size_t rowspan = 1;

    for(size_t i = rowIndex + 1; i < result.rows.size(); i++){
      bool found = false;
      const auto& cells = result.rows[i].cells;

      for(const auto& cell : cells){
        if(cell.verticalMerged &&
           cell.leftBoundary == left &&
           cell.rightBoundary == right)
        {
          found = true;
          break;
        }
      }

      if(!found) break;
      
      rowspan++;
    }

    return rowspan;
  }
  // 檢查函式確認是否是能夠吃掉的 verticalMerged cell 結構體
  bool isCoveredByRowspan(const TableResult& result,
                          size_t rowIndex,
                          size_t left,
                          size_t right)
  {
    if(rowIndex == 0) return false;

    for(size_t r = rowIndex; r > 0; r--){
      const auto& prevRow = result.rows[r - 1];

      for(const auto& cell:prevRow.cells){
        if(cell.leftBoundary == left &&
           cell.rightBoundary == right &&
           cell.verticalMergeStart)
        {
          size_t rowspan = getRowspan(result,r - 1,left,right);

          return (r - 1 + rowspan > rowIndex);
        }
      }
    }

    return false;
  }
  
  void writeResult(const TableResult& result, std::ostream& out){
    std::vector<size_t> grid = getColspanGrid(result);

    out << "<table class=\"rtf-table\">\n";
    // 告知每格的長度
    out << "<colgroup>\n";

    for(size_t i = 1; i < grid.size(); i++){
      size_t width = grid[i] - grid[i - 1];

      out << "  <col style=\"width: "
          << (width / 8)
          << "px;\">\n";
    }

    out << "</colgroup>\n";
    
    const  std::vector<RowResult>& rows = result.rows;
    
    for(size_t i =0;i<rows.size();i++){
      const std::vector<CellResult>& cells = rows[i].cells;

      out << "  <tr>\n";

      // 防呆
      if(cells.empty()){
        out << "  </tr>\n";
        continue;
      }

      size_t left = 0;
      for(size_t j = 0; j < cells.size(); j++){
        const  CellResult& cell = cells[j];

        size_t right = left + cell.width;

        // 如果是上方垂直格的延伸並且通過檢查就不輸出 <td>..</td> 值
        if(cell.verticalMerged && 
           isCoveredByRowspan(result,i,cell.leftBoundary,cell.rightBoundary))
        {
          left = right;
          continue;
        }

        //水平方面需要的資訊
        size_t colspan = getColspan(grid,left,right);

        //垂直方面需要的資訊
        size_t rowspan = 1;
        if(cell.verticalMergeStart){
          rowspan = getRowspan(result,i,cell.leftBoundary,cell.rightBoundary);
        }
        
        out << "    <td";

        if(colspan > 1){
          out << " colspan=\"" << colspan << "\"";
        }

        if(rowspan > 1){
          out << " rowspan=\"" << rowspan << "\"";
        }

        out << ">";
        writeHtmlEscaped(cells[j].text, out);
        out << "</td>\n";

        left = right;
      }

      out << "  </tr>\n";
    }

    out << "</table>\n";
  }

  void renderTable(const TableBlock& table, std::ostream& out){
    auto result = buildResult(table);
    writeResult(result,out);
  }
}