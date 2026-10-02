// =====================================================
// Module : Document (implementation)
// =====================================================
#include "Document.h"
#include<string>
#include<string_view>
#include<vector>
#include<memory>
#include<sstream>
#include<optional>
#include<iostream>
#include<array>

namespace{
  enum class MarkerType{
    // 圖片標記符
    Image,
    ImageErr,

    // 一般功能標記符(換行...等等)
    Par,
    Line,

    // 表格功能標記符
    TableTrowd,
    TableCell,
    TableRow,
    TableIntbl,
    Cellx,
    TrLeft,
    LastRow,
    Itap,
    NestCell,
    NestRow,
    Trhdr,
    Clmgf,
    Clmrg,
    Clvmgf,
    Clvmrg,
    TablePar,
    TableLine,
    NestTablePropsStart,
    NestTablePropsEnd,
    Trrh,

    // 圖形功能標記符
    ShapeStart,
    ShapeEnd,
    ShapeTextStart,
    ShapeTextEnd,
    ShapeTextPar,
    ShapeTextLine,
    ShapeTop,
    ShapeBottom,
    ShapeLeft,
    ShapeRight,
    ShapeZ
  };

  struct MarkerInfo{
    MarkerType type;
    std::string_view text;
    std::optional<int> value = std::nullopt;
  };

  constexpr std::array<MarkerInfo,25> markers{{
    {MarkerType::TableTrowd, "@@RTF_TABLE_TROWD@@"},
    {MarkerType::TableCell , "@@RTF_TABLE_CELL@@" },
    {MarkerType::TableRow  , "@@RTF_TABLE_ROW@@"  },
    {MarkerType::TableIntbl, "@@RTF_TABLE_INTBL@@"},
    {MarkerType::Image     , "@@RTF_IMAGE"       },
    {MarkerType::Par       , "@@par@@"           },
    {MarkerType::Line      , "@@line@@"          },
    {MarkerType::LastRow   , "@@RTF_TABLE_LASTROW@@"},
    {MarkerType::NestCell  , "@@RTF_TABLE_NESTCELL@@"},
    {MarkerType::NestRow   , "@@RTF_TABLE_NESTROW@@"},
    {MarkerType::Trhdr     , "@@RTF_TABLE_TRHDR@@"},
    {MarkerType::Clmgf     , "@@RTF_TABLE_CLMGF@@"},
    {MarkerType::Clmrg     , "@@RTF_TABLE_CLMRG@@"},
    {MarkerType::Clvmgf    , "@@RTF_TABLE_CLVMGF@@"},
    {MarkerType::Clvmrg    , "@@RTF_TABLE_CLVMRG@@"},
    {MarkerType::TablePar  , "@@RTF_TABLE_PAR@@"},
    {MarkerType::TableLine , "@@RTF_TABLE_LINE@@"},
    {MarkerType::NestTablePropsStart , "@@RTF_NEST_TABLE_PROPS_START@@"},
    {MarkerType::NestTablePropsEnd   , "@@RTF_NEST_TABLE_PROPS_END@@"},
    {MarkerType::ShapeStart, "@@RTF_SHAPE_START@@"},
    {MarkerType::ShapeEnd  , "@@RTF_SHAPE_END@@"},
    {MarkerType::ShapeTextStart,"@@RTF_SHAPE_TEXT_START@@"},
    {MarkerType::ShapeTextEnd  ,"@@RTF_SHAPE_TEXT_END@@"},
    {MarkerType::ShapeTextPar  ,"@@RTF_SHAPE_TEXT_PAR@@"},
    {MarkerType::ShapeTextLine ,"@@RTF_SHAPE_TEXT_LINE@@"}
  }};

  std::optional<MarkerInfo> findMarker(std::string_view content, size_t pos) {
    // 先檢查是否是不含數字資訊固定格式標記符
    for (const auto& marker : markers) {
      if (pos + marker.text.size() <= content.size() &&
          content.compare(pos, marker.text.size(), marker.text) == 0) {
        return marker;
      }
    }
    
    // 檢查並處理含有數字資訊之版本的標記符
    // 允許有負號
    // 2026/08/11 因為出現帶有負號的真實案例改成全部允許負號等到使用時再去進行判斷
    // 表格帶數字標記符目標
    constexpr std::string_view trLeftHead = "@@RTF_TABLE_TRLEFT:";
    constexpr std::string_view trrhHead   = "@@RTF_TABLE_TRRH:";
    constexpr std::string_view cellxHead = "@@RTF_TABLE_CELLX:";
    constexpr std::string_view itapHead = "@@RTF_TABLE_ITAP:";

    // 圖形帶數字標記符目標
    constexpr std::string_view shpTopHead    = "@@RTF_SHPTOP:";
    constexpr std::string_view shpBottomHead = "@@RTF_SHPBOTTOM:";
    constexpr std::string_view shpLeftHead   = "@@RTF_SHPLEFT:";
    constexpr std::string_view shpRightHead  = "@@RTF_SHPRIGHT:";
    constexpr std::string_view shpZHead      = "@@RTF_SHPZ:";

    auto  parseNumberMarker = 
    [&content,pos](std::string_view head, MarkerType type, bool allowMinus)
    -> std::optional<MarkerInfo> 
    {
      if(pos + head.size() > content.size() ||
        content.compare(pos, head.size(), head) != 0)
      {
        return std::nullopt;
      }

      size_t numStart = pos + head.size();
      size_t numEnd = numStart;

      if(allowMinus &&
         numEnd < content.size() &&
         content[numEnd] == '-')
      {
        ++numEnd;
      }

      if(numEnd >= content.size() ||
         !std::isdigit(static_cast<unsigned char>(content[numEnd])))
      {
        return std::nullopt;
      }

      while(numEnd < content.size() &&
            std::isdigit(static_cast<unsigned char>(content[numEnd])))
      {
        ++numEnd;
      }

      if(numEnd + 2 > content.size() ||
         content.compare(numEnd, 2, "@@") != 0)
      {
        return std::nullopt;
      }

      int value = std::stoi(std::string(content.substr(numStart, numEnd - numStart)));

      size_t markerEnd = numEnd + 2;

      return MarkerInfo{
        type,
        content.substr(pos, markerEnd - pos),
        value
      };
    };

    if(auto m = parseNumberMarker(trLeftHead,MarkerType::TrLeft,true)){
      return m;
    }

    if(auto m = parseNumberMarker(trrhHead,MarkerType::Trrh,true)){
      return m;
    }
    
    if(auto m = parseNumberMarker(cellxHead,MarkerType::Cellx,true)){
      return m;
    }

    if(auto m = parseNumberMarker(itapHead,MarkerType::Itap,true)){
      return m;
    }

    if(auto m = parseNumberMarker(shpTopHead, MarkerType::ShapeTop, true)){
      return m;
    }

    if(auto m = parseNumberMarker(shpTopHead, MarkerType::ShapeTop, true)){
      return m;
    }

    if(auto m = parseNumberMarker(shpBottomHead, MarkerType::ShapeBottom, true)){
      return m;
    }

    if(auto m = parseNumberMarker(shpLeftHead, MarkerType::ShapeLeft, true)){
      return m;
    }

    if(auto m = parseNumberMarker(shpRightHead, MarkerType::ShapeRight, true)){
      return m;
    }
    
    if(auto m = parseNumberMarker(shpZHead, MarkerType::ShapeZ, true)){
      return m;
    }

    return std::nullopt;
  }
}

// 文字節點函式定義
const std::string& TextNode::text() const{
    return text_;
}

// 圖片節點函式定義
const std::optional<ImageMarkerInfo>& ImageNode::imageMark() const{
  return imageMark_;
}

// 存放文字節點專用的段落空間函式定義
void ParagraphBlock::addText(TextNode text){
    texts_.push_back(std::move(text));
}
const std::vector<TextNode>& ParagraphBlock::texts() const{
    return texts_;
}


// 存放圖片節點專用的段落空間函式定義
const ImageNode& ImageBlock::image() const{
    return image_;
}

// 存放表格區類似一格(1 cell)(最小結構單位)的資訊結構(定義)
const std::string& TableCell::text() const {
  return text_;
}
bool TableCell::empty() const{
  return text_.empty()  && !nestedTable_;
}
bool TableCell::hasNestedTable() const{
  return nestedTable_ != nullptr;
}
const TableBlock* TableCell::nestedTable() const{
  return nestedTable_.get();
}
void TableCell::setNestedTable(std::unique_ptr<TableBlock> table){
  nestedTable_ = std::move(table);
}
void TableCell::setNestedTablePlacement(NestedTablePlacement placement){
  nestedTablePlacement_ = placement;
}
NestedTablePlacement TableCell::nestedTablePlacement() const{
  return nestedTablePlacement_;
}

// 存放表格區一列(1 row)的資訊結構(定義)
void TableRow::addCell(TableCell cell){
  cells_.push_back(std::move(cell));
}
void TableRow::addCellDef(TableCellDef def){
  cellDefs_.push_back(def);
}
void TableRow::setTrleft(int left){
  trLeft_ = left;
}
void TableRow::setLastRow(bool value){
  lastRow_ = value;
}
void TableRow::setHeaderRow(bool value){
  headerRow_ = value;
}
void TableRow::setItap(int value){
  itap_ = value;
}
void TableRow::setTrrh(int value){
  trrh_ = value;
}
const std::vector<TableCell>& TableRow::cells() const {
  return cells_;
}
const std::vector<TableCellDef>& TableRow::cellDefs() const{
  return cellDefs_;
}
int TableRow::trLeft() const{
  return trLeft_;
}
bool TableRow::isLastRow() const{
  return lastRow_;
}
bool TableRow::isHeaderRow() const{
  return headerRow_;
}
int TableRow::Itap() const{
  return itap_;
}
int TableRow::trrh() const{
  return trrh_;
}
bool TableRow::empty() const{
  return cells_.empty();
}

// 存放表格專用的段落空間(定義)
void TableBlock::addRow(TableRow row){
  rows_.push_back(std::move(row));
}
const std::vector<TableRow>& TableBlock::rows() const {
  return rows_;
}
bool TableBlock::empty() const{
  return rows_.empty();
}

// 存放圖形群組(非圖片)專用的段落空間函式定義
const std::string& ShapeTextBlock::text() const {
  return text_;
}
int ShapeTextBlock::top() const{
  return top_;
}
int ShapeTextBlock::bottom() const{
  return bottom_;
}
int ShapeTextBlock::left() const{
  return left_;
}
int ShapeTextBlock::right() const{
  return right_;
}
int ShapeTextBlock::zOrder() const{
  return zOrder_;
}

// 用來存放各種段落空間的容器函式定義
template<typename BlockT,typename... Args>
BlockT& Document::addBlock(Args&&... args){
    auto ptr = std::make_unique<BlockT>(std::forward<Args>(args)...);
    BlockT& ref = *ptr;
    blocks_.push_back(std::move(ptr));
    return ref;
}
const std::vector<std::unique_ptr<Block>>& Document::blocks() const{
    return blocks_;
}

// 將處理好的字串拆解成語意結構函式定義
Document DocumentBuilder::build(const std::string& content){
    Document doc;
    ParagraphBlock* parblock = nullptr;
    
    auto appendText = [&](std::string text) {
      if (text.empty()) return;

      if (!parblock) {
        parblock = &doc.addBlock<ParagraphBlock>(0);
      }

      parblock->addText(TextNode(std::move(text)));
    };

    std::string buffer;
    
    for (size_t i = 0; i < content.size(); ) {

      auto marker = findMarker(content, i);

      if (!marker) {
        buffer.push_back(content[i]);
        ++i;
        continue;
      }

      appendText(std::move(buffer));
      buffer.clear();

      switch(marker -> type){
        case MarkerType::Par :
          parblock = nullptr;
          i += marker->text.size();
          break;
        
        case MarkerType::Line:
          appendText("\n");
          i += marker->text.size();
          break;  

        case MarkerType::TableCell:
        case MarkerType::TableRow:
        case MarkerType::Cellx:
        case MarkerType::TrLeft:
        case MarkerType::LastRow:
        case MarkerType::Itap:
        case MarkerType::NestCell:
        case MarkerType::NestRow:
        case MarkerType::Trhdr:
        case MarkerType::Clmgf:
        case MarkerType::Clmrg:
        case MarkerType::Clvmgf:
        case MarkerType::Clvmrg:
        case MarkerType::TablePar:
        case MarkerType::TableLine:
        case MarkerType::Trrh:
        i += marker->text.size();
        break;

        case MarkerType::TableTrowd:
        case MarkerType::TableIntbl:
        {
          parblock = nullptr;
          std::optional<TableBlockResult> result =
               parseTableBlock(content,i);

          if(!result){
            i += marker->text.size();
            break;
          }     
          doc.addBlock<TableBlock>(std::move(result->table));
          i = result -> nextPos;
          break;
        }  
        case MarkerType::ImageErr:
        case MarkerType::Image:
        {
          parblock = nullptr;
          size_t end = content.find("@@", i + marker->text.size());
          if (end == std::string_view::npos) {
            i += marker->text.size();
            break;
          }

          end += 2;

          std::string_view fullMarker{
            content.data() + i,
            end - i
          };

          auto imgInfo = extractImageId(fullMarker);
          doc.addBlock<ImageBlock>(ImageNode(imgInfo));

          i = end;
          break;
        }
        case MarkerType::ShapeEnd:
        case MarkerType::ShapeTextStart:
        case MarkerType::ShapeTextEnd:
        case MarkerType::ShapeTop:
        case MarkerType::ShapeBottom:
        case MarkerType::ShapeLeft:
        case MarkerType::ShapeRight:
        case MarkerType::ShapeZ:
        i += marker->text.size();
        break;

        case MarkerType::ShapeStart:
        {
          parblock = nullptr;
          std::optional<ShapeTextBlockResult> result = 
             parseShapeBlock(content, i);

          if(!result){
              i += marker->text.size();
              break;
          }

          doc.addBlock<ShapeTextBlock>(std::move(result->shape));

          i = result->nextPos;
          break;
        }
        
        default:
          i += marker->text.size();
          break;
      }
    }

    appendText(std::move(buffer));
    return doc;
}
std::optional<ImageMarkerInfo> DocumentBuilder::extractImageId(std::string_view text){
    constexpr std::string_view imgHead = "@@RTF_IMAGE";
    constexpr std::string_view imgErr  = "_ERR_";
    ImageMarkerInfo info;
    
    std::size_t pos = text.find(imgHead);
    if(pos == std::string::npos){
      return std::nullopt;
    }
    
    std::size_t cur = pos + imgHead.size();
    if(cur >= text.size()) return std::nullopt;
    
    
    if(cur + imgErr.size() <= text.size() &&
      text.compare(cur, imgErr.size(), imgErr) == 0){
      cur += imgErr.size();
      info.type = ImageMarkerType::Error;
      
    }else if(text[cur] == '_')
    {
      cur++;
      info.type = ImageMarkerType::Normal;
    }else{
      return std::nullopt;
    }
    
    std::size_t start = cur;
    while(cur < text.size() &&
        std::isdigit(static_cast<unsigned char>(text[cur]))){
        ++cur;
    }

    // 至少要有一位數字
    if(start == cur){
      return std::nullopt;
    }
    
    if(cur + 1 >= text.size() ||
       text[cur] != '@' ||
       text[cur + 1] != '@'){
        return std::nullopt;
    }

    info.id = text.substr(start, cur - start);
    
    return info;
}

// 結尾檢查
void trimTrailingNewline(std::string& s) {
  while (!s.empty() && s.back() == '\n'){
    s.pop_back();
  }
};

// 暫存機制使用的結構
struct PendingRowState {
  int itap = 1;
  bool itapSeen = false;

  TableRow row;
  std::string cellText;
  TableCellDef pendingDef;

  // 表格暫存成員 每次遇到 cell 結束時的標記符都檢查是否需要掛載
  std::unique_ptr<TableBlock> pendingNestedTable;

  // 用來判斷 cell 表格內的順序用
  NestedTablePlacement pendingNestedTablePlacement =
        NestedTablePlacement::None;
}; 

//將標記符轉換成語意結構時使用的 struct
struct TableParseContext {
  explicit TableParseContext(int level = 1): itap(level){}
  
  int itap = 1;
  TableBlock table;

  // 每一層暫停時保留的解析狀態
  PendingRowState savePending;
  bool hasSavePending = false;
}; 
// 用來判斷 intbl 後面是否跟著 itap 控制符以及控制符數字
std::optional<int> peekImmediateItap(
    std::string_view text,
    size_t pos)
{
    auto marker = findMarker(text, pos);

    if(!marker){
        return std::nullopt;
    }

    if(marker->type != MarkerType::Itap){
        return std::nullopt;
    }

    if(!marker->value){
        return std::nullopt;
    }

    return *marker->value;
}
// 巢狀表格退層時移動表格資料到對應層數的暫存器中並移除對應層數
bool leaveOneTableLevel(
    std::vector<TableParseContext>& stack,
    PendingRowState& pending)
{
  // 防呆
  if(stack.size() <= 1){
    return false;
  }

  auto& parent = stack[stack.size() - 2];

  if(!parent.hasSavePending){
    return false;
  }

  // 暫存資料並移除子層
  TableBlock childTable = std::move(stack.back().table);
  stack.pop_back();

  // 將父層的暫存資料移動給共用暫存器
  pending = std::move(stack.back().savePending);

  // 更新陣列中的資料
  stack.back().savePending = PendingRowState{};
  stack.back().hasSavePending = false;

  // 依據暫存器內是否有儲存文字資料決定順序
  pending.pendingNestedTablePlacement =
    pending.cellText.empty()
        ? NestedTablePlacement::BeforeText
        : NestedTablePlacement::AfterText;

  pending.pendingNestedTable =
  std::make_unique<TableBlock>(
    std::move(childTable)
  );

  return true;
}
// 遇到 cell 結束標記符時的共用函式
void finalizePendingCell(PendingRowState& pending)
{
  trimTrailingNewline(pending.cellText);

  TableCell currentCell(std::move(pending.cellText));

  if(pending.pendingNestedTable){
    currentCell.setNestedTable(
      std::move(pending.pendingNestedTable)
    );

    currentCell.setNestedTablePlacement(pending.pendingNestedTablePlacement);
  }

  pending.row.addCell(std::move(currentCell));

  // move 後的 string 已合法但狀態未指定，明確清空較清楚
  pending.cellText.clear();
  pending.pendingNestedTablePlacement = NestedTablePlacement::None;
}
// parseTableBlock 函式回傳前最後的檢查函示用來應對空層狀態的存在
TableBlock takeFinalTable(
    std::vector<TableParseContext>& stack,
    PendingRowState& pending)
{
  // 防呆
  if(stack.empty()){
    return TableBlock{};
  }

  // 正常外層已正式完成。
  if(!stack.front().table.rows().empty()){
      return std::move(stack.front().table);
  }

  // 子表格已退回，但外層從未完成 cell/row。
  // 視為截取出的巢狀表格片段，提升子表格為根。
  if(pending.pendingNestedTable){
    TableBlock result = std::move(*pending.pendingNestedTable);

    pending.pendingNestedTable.reset();
    return result;
  }

  // 解析結束時仍停在較深層。
  // 選擇最深且有實際 row 的 context。
  for(auto it = stack.rbegin();
      it != stack.rend();
      ++it)
  {
    if(!it->table.rows().empty()){
      return std::move(it->table);
    }
  }

  return TableBlock{};
}

std::optional<TableBlockResult>
DocumentBuilder::parseTableBlock(std::string_view text,size_t pos){
  // 先檢查並跳過開頭標記符
  size_t i = pos;

  auto first = findMarker(text,i);
  if(!first ||
    (first->type != MarkerType::TableTrowd &&
     first->type != MarkerType::TableIntbl))
  {
    return std::nullopt;
  }

  // 真實使用的資訊陣列
  std::vector<TableParseContext> stack;
  stack.emplace_back(1);

  // 公用的區域暫存器
  PendingRowState pending;

  // 用於判斷是否在巢狀迴圈範圍內的指標
  bool insideNestTableProps = false;

  // 避免連續換行被重複塞入
  auto appendCellNewline = [&pending]() {
    if (!pending.cellText.empty() && pending.cellText.back() != '\n') {
      pending.cellText.push_back('\n');
    }
  };
  

  for(;i<text.size();){
    auto marker = findMarker(text, i);

    if (!marker) {
      pending.cellText.push_back(text[i]);
      ++i;
      continue;
    }

    switch(marker -> type){
      case MarkerType::TableCell : 
      {
        finalizePendingCell(pending);

        i += marker -> text.size();
        continue;
      } 
      case MarkerType::TableRow :
      {
        if(!pending.cellText.empty()){
          trimTrailingNewline(pending.cellText);
          
          TableCell currentCell(std::move(pending.cellText));
          pending.row.addCell(std::move(currentCell));

          pending.cellText.clear();
        }

        if(!pending.row.empty()){
          stack.back().table.addRow(std::move(pending.row));
        }

        pending = PendingRowState{};
        
        i += marker -> text.size();

        auto next = findMarker(text,i);

        if(next &&
          (next->type == MarkerType::TableTrowd ||
           next->type == MarkerType::TableIntbl))
        {
          continue;
        }

        // 回傳前用函式對容器做最後檢查避免空層
        TableBlock finalTable = takeFinalTable(stack, pending);

        return TableBlockResult{std::move(finalTable),i};
      }
      case MarkerType::TableTrowd:
      {
        i += marker -> text.size();
        continue;
      }
      case MarkerType::TableIntbl:
      {
        const size_t nextPos = i + marker->text.size();

        if(auto targetItap = peekImmediateItap(text,nextPos)){
          int currentItap = stack.back().itap;

          if(*targetItap > currentItap){
            // push 新 PendingRowState
            auto& parentCtx = stack.back();

            // 把共用暫存器存入目前層級並清空
            parentCtx.savePending = std::move(pending);
            parentCtx.hasSavePending = true;

            pending = PendingRowState{};

            // 新增對應層級
            stack.emplace_back(*targetItap);

            // 設定共用暫存器
            pending.itap = *targetItap;
            pending.itapSeen = true;
            pending.row.setItap(*targetItap);
          }
          else if(*targetItap < currentItap){
            if(!leaveOneTableLevel(stack,pending)){
              // 如果加入 log 系統可在 log 中加入訊息
              // 目前只有先執行退層函式
            }
            
          }
         
        }else if(stack.size() > 1){
          if(!leaveOneTableLevel(stack,pending)){
            // 如果加入 log 系統可在 log 中加入訊息
            // 目前只有先執行退層函式
          }
        }

        i = nextPos;
        continue;
      }
      case MarkerType::LastRow:
      {
        pending.row.setLastRow();

        i += marker ->text.size();
        continue;
      }
      case MarkerType::NestCell:
      {
        finalizePendingCell(pending);
        
        i += marker -> text.size();
        continue;
      }
      case MarkerType::NestRow:
      {
        if(!pending.cellText.empty()){
          trimTrailingNewline(pending.cellText);
          
          TableCell currentCell(std::move(pending.cellText));
          pending.row.addCell(std::move(currentCell));

          pending.cellText.clear();
        }

        if(!pending.row.empty()){
          stack.back().table.addRow(std::move(pending.row));
        }

        const int currentItap = pending.itap;

        pending = PendingRowState{};

        pending.itap = currentItap;
        pending.itapSeen = true;
        pending.row.setItap(currentItap);
        
        i += marker ->text.size();
        continue;
      }
      case MarkerType::Trhdr:
      {
       pending.row.setHeaderRow();
        i += marker ->text.size();
        continue;
      }
      case MarkerType::Clmgf:
      {
        pending.pendingDef.horizontalMergeStart = true;
        i += marker ->text.size();
        continue;
      }
      case MarkerType::Clmrg:
      {
        pending.pendingDef.horizontalMerged = true;
        i += marker ->text.size();
        continue;
      }
      case MarkerType::Clvmgf:
      {
        pending.pendingDef.verticalMergeStart = true;
        i += marker ->text.size();
        continue;
      }
      case MarkerType::Clvmrg:
      {
        pending.pendingDef.verticalMerged = true;
        i += marker ->text.size();
        continue;
      }
      case MarkerType::Cellx:
      {
        if(marker->value){
          pending.pendingDef.cellx = *marker->value;
          pending.row.addCellDef(pending.pendingDef);
          pending.pendingDef = TableCellDef{};
        }
        i += marker ->text.size();
        continue;
      }
      case MarkerType::TrLeft:
      {
        if(marker->value){
          pending.row.setTrleft(*marker->value);
        }
        i += marker ->text.size();
        continue;
      }
      case MarkerType::Itap:
      {
        if(marker->value){
          const int level = *marker->value;
          pending.itap = level;
          pending.itapSeen = true;
          pending.row.setItap(level);
        }
        i += marker ->text.size();
        continue;
      }
      case MarkerType::Trrh:
      {
        if(marker->value){
          pending.row.setTrrh(*marker->value);
        }
        i += marker ->text.size();
        continue;
      }
      case MarkerType::TablePar:
      case MarkerType::TableLine:
      case MarkerType::Par:
      case MarkerType::Line:
      {
        appendCellNewline();
        i += marker ->text.size();
        continue;
      }
      case MarkerType::NestTablePropsStart:
      {
        insideNestTableProps = true;
        i += marker->text.size();
        continue;
      }
      case MarkerType::NestTablePropsEnd:
      {
        insideNestTableProps = false;
        i += marker->text.size();
        continue;
      }
      case MarkerType::Image:
      case MarkerType::ImageErr:
      {
        size_t end = text.find("@@", i + marker->text.size());
        if (end == std::string_view::npos) {
          i += marker->text.size();
          continue;
        }

        end += 2;

        std::string_view fullMarker{
          text.data() + i,
          end - i
        };

        pending.cellText.append(fullMarker);
        
        i = end;
        continue;
      }
      default :
      {
        if(!stack.back().table.empty()){
          return TableBlockResult{std::move(stack.back().table),i};
        }
        return std::nullopt;
      }
    }
  }

  if(!pending.cellText.empty()){
    trimTrailingNewline(pending.cellText);
    pending.row.addCell(TableCell(std::move(pending.cellText)));
  }

  if(!pending.row.empty()){
    stack.back().table.addRow(std::move(pending.row));
  }

  // 此處是迴圈完整跑完遇到文字結尾的情況前面會有提前回傳
  if(!stack.back().table.empty()){
    return TableBlockResult{std::move(stack.back().table),i};
  }

  return std::nullopt;
}

std::optional<ShapeTextBlockResult> 
DocumentBuilder::parseShapeBlock(std::string_view text,size_t pos){
   
  // 確認是正確的 start 標記符開頭 
  auto startMarker = findMarker(text, pos);

  if(!startMarker ||startMarker->type != MarkerType::ShapeStart){
    return std::nullopt;
  }

  size_t i = pos + startMarker->text.size();

  std::string shapetext;

  int top = 0;
  int bottom = 0;
  int left = 0;
  int right = 0;
  int zOrder = 0;

  while(i < text.size()){

    auto marker = findMarker(text, i);

    if(!marker){
      ++i;
      continue;
    }

    if(marker->type == MarkerType::ShapeEnd){
      i += marker->text.size();

      ShapeTextBlock shape(shapetext,
                           top,
                           bottom,
                           left,
                           right,
                           zOrder);

      return ShapeTextBlockResult{
        std::move(shape),
        i
      };
    }

    switch(marker->type){
      case MarkerType::ShapeTop:
      {
        if(marker->value){
          top = *marker->value;
        }
        i += marker ->text.size();
        continue;
      }
      case MarkerType::ShapeBottom:
      {
        if(marker->value){
          bottom = *marker->value;
        }
        i += marker ->text.size();
        continue;
      }
      case MarkerType::ShapeLeft:
      {
        if(marker->value){
          left = *marker->value;
        }
        i += marker ->text.size();
        continue;
      }
      case MarkerType::ShapeRight:
      {
        if(marker->value){
          right = *marker->value;
        }
        i += marker ->text.size();
        continue;
      }
      case MarkerType::ShapeZ:
      {
        if(marker->value){
          zOrder = *marker->value;
        }
        i += marker ->text.size();
        continue;
      }
      
      case MarkerType::ShapeTextStart:
      {
        i += marker->text.size();

        // 開始吃文字區塊
        while(i < text.size()){
          auto textMarker = findMarker(text, i);

          if(!textMarker){
            shapetext.push_back(text[i]);
            i++;
            continue;
          }

          if(textMarker->type == MarkerType::ShapeTextEnd){
            i += textMarker->text.size();
            break;
          }
          
          // 目前觀察實際上沒有換行動作先保留
          if(textMarker->type == MarkerType::ShapeTextPar){
            i += textMarker->text.size();
            continue;
          }
          // 目前觀察實際上沒有換行動作先保留 
          if(textMarker->type == MarkerType::ShapeTextLine){
            i += textMarker->text.size();
            continue;
          }

          // 其餘標記符此範圍內遇到先直接吃掉
          i += textMarker->text.size();
        }
        
        break;
      }   
      
      default:
      {
        i += marker->text.size();
        continue;
      }
    }
  }
  
  return std::nullopt;
}