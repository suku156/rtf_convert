// =====================================================
// Module : MdTableRenderer (implementation)
// md 表格格式規定要有header ,目前沒有相關設置直接拿第一列當 header
// =====================================================
#include "MdTableRenderer.h"
#include "SemanticStructure_Module/Document.h"
#include<ostream>
#include<string>
#include<cstddef>
#include <utility>
#include <set>
#include<algorithm>

namespace md_table {
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
        
        cellResult.text = cells[i].text();
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
 
  // 負責寫入 md 表格格式中的 Separator
  std::string writeSeparator(size_t columnCount){
    std::string sep;
    sep.append("|");

    for(size_t i = 0; i < columnCount; ++i){
      sep.append(" ");
      sep.append("--- |");
    }

    return sep;
  }
  // 依據 得到的列 的資訊實際畫出列
  std::string writeRow(
    const std::vector<std::string>& cells)
  {
    std::string result = "|";

    for(const auto& cell : cells){
      result += " ";
      result += cell;
      result += " |";
    }

    return result;
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

  // MD cell 專用格式化函式
  // 處理換行符等符號到應對的 md 格式中
  std::string formatMdCellText(const std::string& text)
  {
    std::string result;
    result.reserve(text.size());

    for(char c : text){
      switch(c){
        case '\n':
            result += "<br>";
            break;

        case '|':
            result += "\\|";
            break;

        case '\r':
            // 忽略 CR
            break;

        default:
            result += c;
            break;
      }
    }

    return result;
  }

  // 把遞迴表格的內容取出
  //  md 表格不支援巢狀表格優先確保 資訊完整度
  std::string flattenNestedTable(const TableBlock& table)
  {
    TableResult nestedResult = buildResult(table);

    std::string result;

    for(const RowResult& row : nestedResult.rows){
        bool firstCell = true;

        for(const CellResult& cell : row.cells){
            if(!firstCell){
                result += " / ";
            }

            std::string Fcelltext = formatMdCellText(cell.text);

            result += Fcelltext;

            if(cell.nestedTable){
                if(!cell.text.empty()){
                    result += "<br>";
                }

                result += flattenNestedTable(*cell.nestedTable);
            }

            firstCell = false;
        }

        result += "<br>";
    }

    return result;
  }

  

  std::string buildMdCellContent(const CellResult& cell)
  {
    std::string text = formatMdCellText(cell.text);

    if(!cell.nestedTable){
        return text;
    }

    std::string nested = flattenNestedTable(*cell.nestedTable);

    switch(cell.nestedTablePlacement){
        case NestedTablePlacement::BeforeText:
            if(text.empty()){
                return nested;
            }
            return nested + "<br>" + text;

        case NestedTablePlacement::AfterText:
            if(text.empty()){
                return nested;
            }
            return text + "<br>" + nested;

        default:
            if(text.empty()){
                return nested;
            }
            return text + "<br>" + nested;
    }
  }

  // 讓每一列 依據 共用水平線為依據產生 可畫出的結果
  std::vector<std::string> expandRowByGrid(const RowResult& row,const std::vector<int>& grid)
  {
    if(grid.size() < 2){
      return {};
    }

    std::vector<std::string> mdCells(grid.size() - 1);

    for(const CellResult& cell : row.cells){
      auto it = std::find(
        grid.begin(),
        grid.end(),
        cell.leftBoundary
      );

      if(it == grid.end()){
        continue;
      }

      size_t index = static_cast<size_t>(std::distance(grid.begin(), it));

      if(index >= mdCells.size()){
         continue;
      }

      mdCells[index] = buildMdCellContent(cell);
    }

    return mdCells;
  }
  

  void writeResult(const TableResult& result, std::ostream& out){
    if(result.rows.empty()){
      return;
    }
    
    std::vector<int> grid = getColspanGrid(result);
    
    if(grid.size() < 2){
      return;
    }

    size_t columnCount = grid.size() - 1;
    
    for(size_t i = 0; i < result.rows.size(); ++i){
      std::vector<std::string> mdCells = expandRowByGrid(result.rows[i],grid);

      out << writeRow(mdCells) << '\n';

      if(i == 0){
        out << writeSeparator(columnCount)
                << '\n';
      }
    }
  }

  
  // 外部呼叫的函式入口
  void renderTable(const TableBlock& table, std::ostream& out){
    auto result = buildResult(table);
    writeResult(result,out);
  }
   
}