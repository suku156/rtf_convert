// =====================================================
// Module : HtmlTableRenderer (implementation)
// =====================================================

#include "HtmlTableRenderer.h"
#include "SemanticStructure_Module/Document.h"
#include <ostream>
#include <string>
#include <cstddef>
#include <utility>
#include <set>
#include <optional>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <charconv>
#include <system_error>
#include <cctype>

namespace html_table{
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
        
        const auto& cellText = cells[i].text();
        
        cellResult.text = cellText;
        
        
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

  // 新增：處理一般文字與圖片標記
  void writeHtmlEscapedWithImages(
    std::string_view text,
    std::ostream& out,
    const std::unordered_map<int, std::filesystem::path>& index)
  {
    constexpr std::string_view imgHead = "@@RTF_IMAGE";
    constexpr std::string_view imgErr  = "_ERR_";

    size_t plainStart = 0;

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

          //確認找到標記符後先輸出掉之前的文字
          writeHtmlEscaped(text.substr(plainStart, i - plainStart), out);

          // 擷取數字部份
          std::string num(text.data() + numStart,MarkerEnd - numStart);

          out << "<div class=\"image-block\">\n";
          out << "[IMAGE_ERR :  ";
          out <<  num ;
          out <<  "]";
          out <<  "</div>\n";

          i = MarkerEnd + 1; // 跳過最後兩個 @@ 迴圈會在自動加一所以只加一
          plainStart = i + 1;
          continue;

        }else if( j < text.size() && text[j] == '_'){
          size_t numStart = j + 1;
          
          size_t end = text.find("@@",numStart);
          
          if(end == std::string::npos){
            i = numStart - 1; // 迴圈會自動加
            continue;
          }

          //確認找到標記符後先輸出掉之前的文字
          writeHtmlEscaped(text.substr(plainStart, i - plainStart), out);

          // 擷取數字部份
          std::string num(text.data() + numStart,end - numStart);

          int imageNumber = 0;

          const auto [ptr, ec] = std::from_chars(
            num.data(),
            num.data() + num.size(),
            imageNumber
          );

          if (ec == std::errc{} &&
            ptr == num.data() + num.size() &&
            imageNumber >= 0)
          {
            const auto it = index.find(imageNumber);

            if(it != index.end()){
              const auto& path = it->second;
              std::string ext = path.extension().string();

              std::transform(ext.begin(), ext.end(), ext.begin(),
                  [](unsigned char c) {
                    return static_cast<char>(std::tolower(c));
                  }
              );

              if (ext == ".wmf" || ext == ".emf")
              {
                out <<  "<div class=\"image-unsupported\">\n";
                out <<  "[圖片無法直接顯示：";
                out <<  path.filename().string();
                out <<  "，格式尚未支援]\n";
                out <<  "</div>\n";
              }else{
                const std::string src =
                "images/" + it->second.filename().generic_string();

                out <<  "<div class=\"image-block\">\n";
                out <<  "<img src=\"";
                out <<  src;
                out <<  "\" alt=\"RTF image\">\n";
                out <<  "</div>\n";
              }
            }else{
              out <<  "<div class=\"image-missing\">"
                          "[找不到圖片：";
              out <<  num;
              out <<  "]</div>\n";
            }
          }else{
            out <<  "<div class=\"image-missing\">"
                      "[無效的圖片編號]</div>\n";
          }

          i = end + 1;
          plainStart = i + 1;
          continue;
        }
      }
    }
    // 迴圈結束後，處理剩餘的一般文字
    writeHtmlEscaped(text.substr(plainStart), out);
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
  // 用來查找在共用水平座標列的位置
  std::optional<size_t> findGridIndex(const std::vector<int>& grid, size_t value){
    auto it = std::find(grid.begin(), grid.end(), value);

    if(it == grid.end()){
      return std::nullopt;
    }

    return static_cast<size_t>(std::distance(grid.begin(), it));
  }
  // 使用流程中的小工具用來回傳要跨幾格(水平線用)
  size_t getColspan(const std::vector<int>& grid,int left,int right){
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
    int rightBoundary = 0;
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
                    int left,
                    int right)
  {
    size_t rowspan = 1;

    for(size_t i = rowIndex + 1; i < result.rows.size(); i++){
      const auto& row   = result.rows[i];
      bool found = false;

      for(size_t j = 0; j <row.cells.size();){
        const auto& cell = row.cells[j];

        HorizontalMergeInfo horizontalInfo =
                getHorizontalMergeInfo(row, j);

        
        int currentLeft = cell.leftBoundary;
        int currentRight = cell.rightBoundary;

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
                          int left,
                          int right)
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

        int prevLeft = prevCell.leftBoundary;
        int prevRight = prevCell.rightBoundary;

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
  
  void writeResult(const TableResult& result, std::ostream& out,
                   const std::unordered_map<int, std::filesystem::path>& index)
  {
    std::vector<int> grid = getColspanGrid(result);

    
    out << "<table class=\"rtf-table\">\n";
    // 告知每格的長度
    out << "<colgroup>\n";

    for(size_t i = 1; i < grid.size(); i++){
      int width = grid[i] - grid[i - 1];

      if(width <= 0){
        continue;
      }

      out << "  <col style=\"width: "
          << (static_cast<double>(width) / 8.0)
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

        int left  = cell.leftBoundary;
        int right = cell.rightBoundary;

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
        size_t colspan = 1;
        if(right >= left){
          colspan = getColspan(grid,left,right);
        }
        
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
        
        if(cell.nestedTable){
          switch(cell.nestedTablePlacement){
            case NestedTablePlacement::BeforeText:
              renderTable(*cell.nestedTable, out,index);
              writeHtmlEscapedWithImages(cell.text, out,index);
            break;
            case NestedTablePlacement::AfterText:
              writeHtmlEscapedWithImages(cell.text, out,index);
              renderTable(*cell.nestedTable, out,index);
            break;
            case NestedTablePlacement::None:
              // 狀態不一致：
              // 有 nestedTable，卻沒有設定位置。
              // 第一版可選擇保守預設為文字後。
              writeHtmlEscapedWithImages(cell.text, out,index);
              renderTable(*cell.nestedTable, out,index);
            break;
          }
        }else{
          writeHtmlEscapedWithImages(cell.text, out,index);
        }

        out << "</td>\n";

        j += horizontalInfo.consumedCells;
      }

      out << "  </tr>\n";
    }

    out << "</table>\n";
  }

  void renderTable(const TableBlock& table, std::ostream& out,
                   const std::unordered_map<int, std::filesystem::path>& index)
  {
    auto result = buildResult(table);
    writeResult(result,out,index);
  }
}