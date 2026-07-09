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

namespace md_table {
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
  // 負責寫入 md 表格格式中的 header
  std::string writeHeader(const std::vector<CellResult>& cells){
    std::string headerText;
    headerText.append("|");
    
    for(size_t j =0;j < cells.size();j++){
      const CellResult& cell = cells[j];
      
      headerText.append(" ");
      headerText.append(cell.text);
      headerText.append(" |");
    }

    return headerText;
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
  // 負責寫入 md 表格格式中的 body
  std::string writeBody(const std::vector<CellResult>& cells){
    std::string bodyText;
    bodyText.append("|");

    for(size_t j =0;j < cells.size();j++){
      const CellResult& cell = cells[j];
      
      bodyText.append(" ");
      bodyText.append(cell.text);
      bodyText.append(" |");
    }

    return bodyText;
  }

  void writeResult(const TableResult& result, std::ostream& out){
    const std::vector<RowResult>& rows = result.rows;
    std::vector<std::string> mdResult;
    // 會多一列 Separator
    mdResult.reserve(rows.size() + 1);

    for(size_t i =0;i<rows.size();i++){
      const std::vector<CellResult>& cells = rows[i].cells;
      
      // 先完成 md　表格格式需要的　header　與 Separator 部份
      if(i == 0){
        mdResult.push_back(writeHeader(cells));
        mdResult.push_back(writeSeparator(cells.size()));
        continue;
      }

      mdResult.push_back(writeBody(cells));
    }

    for(const std::string& line : mdResult){
      out << line << '\n';
    }
  }

  
  // 外部呼叫的函式入口
  void renderTable(const TableBlock& table, std::ostream& out){
    auto result = buildResult(table);
    writeResult(result,out);
  }
   
}