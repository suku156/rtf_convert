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
#include<cstdlib>

namespace html_table{
  struct CellResult {
    std::string text;
    size_t width = 0;

    // 水平線的左右邊界(共用水平參考線用)
    size_t leftBoundary = 0;
    size_t rightBoundary = 0;

    // 垂直合併用數據
    bool verticalMergeStart = false;
    bool verticalMerged = false;

    // 水平合併用數據
    bool horizontalMergeStart = false;
    bool horizontalMerged = false;
  };

  struct RowResult {
    std::vector<CellResult> cells;
    // 紀錄每一列最後一個 cellx , 也就是該列右邊界
    size_t rightBoundary = 0;
    int trrh = 0;
  };

  struct TableResult {
    std::vector<RowResult> rows;
  };  
  
  TableResult buildResult(const TableBlock& table){
    TableResult tableResult;
    tableResult.rows.reserve(table.rows().size());

    for(const auto& row : table.rows()){
      RowResult rowResult;

      rowResult.trrh = row.trrh();
      
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
          const auto rawCellx = cellDefs[i].cellx;

          if(rawCellx >= 0){
            size_t currentCellx = static_cast<size_t>(rawCellx);

            cellResult.rightBoundary = currentCellx;

            if(currentCellx >= prevCellx){
              cellResult.width = currentCellx - prevCellx;
            }else{
              cellResult.width = 0;
            }

            prevCellx = currentCellx;
          }
         
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

  // html 需要的安全符號替換
  void writeHtmlEscaped(std::string_view text, std::ostream& out){
    for(char c : text){
        switch(c){
        case '&': out << "&amp;"; break;
        case '<': out << "&lt;"; break;
        case '>': out << "&gt;"; break;
        case '"': out << "&quot;"; break;
        case '\'': out << "&#39;"; break;
        case '\n': out << "<br>\n"; break;
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
  // 水平合併判斷函式回傳結構
  struct HorizontalMergeInfo {
    // 合併結尾的右邊界
    size_t rightBoundary = 0;
    // 總共合併掉幾格(使用迴圈以此數推進基本要給 1 避免迴圈卡住)
    size_t consumedCells = 1;
  };
  // 水平合併格判斷函式
  HorizontalMergeInfo getHorizontalMergeInfo(const RowResult& row,size_t cellIndex){
    HorizontalMergeInfo info;
    // 保險再做一次 避免迴圈卡住
    info.consumedCells = 1;

    if(cellIndex >= row.cells.size()){
      return info;
    }

    const auto& startCell = row.cells[cellIndex];

    info.rightBoundary = startCell.rightBoundary;

    if(!startCell.horizontalMergeStart){
      return info;
    }

    for(size_t i = cellIndex + 1;
        i < row.cells.size();
        ++i)
    {
        const auto& nextCell = row.cells[i];

        if(!nextCell.horizontalMerged){
          break;
        }

        // 防止異常、不連續邊界被錯誤合併
        if(nextCell.leftBoundary != info.rightBoundary){
          break;
        }

        info.rightBoundary = nextCell.rightBoundary;
        info.consumedCells++;
    }

    return info;
  }
  // 推算要跳過幾格的函式(垂直線用)
  size_t getRowspan(const TableResult& result,
                    size_t rowIndex,
                    size_t left,
                    size_t right)
  {
    size_t rowspan = 1;

    for(size_t i = rowIndex + 1; i < result.rows.size(); i++){
      const auto& row   = result.rows[i];
      bool found = false;

      for(size_t j = 0; j <row.cells.size();){
        const auto& cell = row.cells[j];

        HorizontalMergeInfo horizontalInfo =
                getHorizontalMergeInfo(row, j);

        
        size_t currentLeft = cell.leftBoundary;
        size_t currentRight = cell.rightBoundary;

        if(cell.horizontalMergeStart){
          currentRight = horizontalInfo.rightBoundary;
        }
        
        if(cell.verticalMerged &&
           currentLeft == left &&
           currentRight == right)
        {
          found = true;
          break;
        }

        j += horizontalInfo.consumedCells;
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
    if(rowIndex == 0){
      return false;
    }

    for(size_t r = rowIndex; r > 0; --r){
      const auto& prevRow = result.rows[r - 1];

      for(size_t j = 0; j < prevRow.cells.size();){
        const auto& prevCell = prevRow.cells[j];

        HorizontalMergeInfo horizontalInfo =
              getHorizontalMergeInfo(prevRow, j);

        size_t prevLeft = prevCell.leftBoundary;
        size_t prevRight = prevCell.rightBoundary;

        if(prevCell.horizontalMergeStart){
          prevRight = horizontalInfo.rightBoundary;
        }

        if(prevLeft == left &&
           prevRight == right &&
           prevCell.verticalMergeStart)
        {
          size_t rowspan = getRowspan(result,r - 1,prevLeft,prevRight);
          return (r - 1 + rowspan > rowIndex);
        }

        j += horizontalInfo.consumedCells;
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
      const RowResult& row = rows[i];
      const std::vector<CellResult>& cells = row.cells;

      out << "  <tr";

      if(row.trrh != 0){
        // htnl視覺補償倍率
        constexpr double scale = 1.1;

        double pt = std::abs(row.trrh) / 20.0;
        pt *= scale;
       
        out << " style=\"height:" << pt << "pt;\"";
      }  
      
      out << ">\n";

      // 防呆
      if(cells.empty()){
        out << "  </tr>\n";
        continue;
      }

      
      for(size_t j = 0; j < cells.size();){
        const  CellResult& cell = cells[j];

        size_t left  = cell.leftBoundary;
        size_t right = cell.rightBoundary;

        // 檢查並回傳水平合併資訊
        HorizontalMergeInfo horizontalInfo = 
                getHorizontalMergeInfo(row,j);

        if(cell.horizontalMergeStart){
          right = horizontalInfo.rightBoundary;
        }

        // 如果是上方垂直格的延伸並且通過檢查就不輸出 <td>..</td> 值
        if(cell.verticalMerged && 
           isCoveredByRowspan(result,i,left,right))
        {
          j += horizontalInfo.consumedCells;
          continue;
        }

        //水平方面需要的資訊
        size_t colspan = getColspan(grid,left,right);

        //垂直方面需要的資訊
        size_t rowspan = 1;
        if(cell.verticalMergeStart){
          rowspan = getRowspan(result,i,left,right);
        }
        
        out << "    <td";

        if(colspan > 1){
          out << " colspan=\"" << colspan << "\"";
        }

        if(rowspan > 1){
          out << " rowspan=\"" << rowspan << "\"";
        }

        out << ">";
        writeHtmlEscaped(cell.text, out);
        out << "</td>\n";

        j += horizontalInfo.consumedCells;
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