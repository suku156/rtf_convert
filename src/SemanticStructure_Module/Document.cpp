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
    TableTrowd,
    TableCell,
    TableRow,
    TableIntbl,
    Image,
    ImageErr,
    Par,
    Line,
    Cellx,
    TrLeft,
    LastRow,
    Itap,
    NestCell,
    NestRow,
    Trhdr,
    Clmgf,
    Clmrg
  };

  struct MarkerInfo{
    MarkerType type;
    std::string_view text;
    std::optional<int> value = std::nullopt;
  };

  constexpr std::array<MarkerInfo,13> markers{{
    {MarkerType::TableTrowd, "@@RTF_TABLE_TROWD@@"},
    {MarkerType::TableCell , "@@RTF_TABLE_CELL@@" },
    {MarkerType::TableRow  , "@@RTF_TABLE_ROW@@"  },
    {MarkerType::TableIntbl, "@@RTF_TABLE_INTBL@@"},
    {MarkerType::Image     , "@@RTF_IMAGE"       },
    {MarkerType::Par       , "@@par@@"            },
    {MarkerType::Line      , "@@line@@"           },
    {MarkerType::LastRow   , "@@RTF_TABLE_LASTROW@@"},
    {MarkerType::NestCell  , "@@RTF_TABLE_NESTCELL@@"},
    {MarkerType::NestRow   , "@@RTF_TABLE_NESTROW@@"},
    {MarkerType::Trhdr     , "@@RTF_TABLE_TRHDR@@"},
    {MarkerType::Clmgf     , "@@RTF_TABLE_CLMGF@@"},
    {MarkerType::Clmrg     , "@@RTF_TABLE_CLMRG@@"}
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
    constexpr std::string_view trLeftHead = "@@RTF_TABLE_TRLEFT:";
    // 不允許有負號
    constexpr std::string_view cellxHead = "@@RTF_TABLE_CELLX:";
    constexpr std::string_view itapHead = "@@RTF_TABLE_ITAP:";

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

    if(auto m = parseNumberMarker(cellxHead,MarkerType::Cellx,false)){
      return m;
    }

    if(auto m = parseNumberMarker(trLeftHead,MarkerType::TrLeft,true)){
      return m;
    }

    if(auto m = parseNumberMarker(itapHead,MarkerType::Itap,false)){
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

// 存放表格區類似一格(最小結構單位)的資訊結構(定義)
const std::string& TableCell::text() const {
  return text_;
}
bool TableCell::empty() const{
  return text_.empty();
}
void TableCell::setMergeStart(bool value){
  mergeStart_ = value;
}
void TableCell::setMerged(bool value){
  merged_ = value;
}
bool TableCell::isMergeStart() const{
  return mergeStart_;
}
bool TableCell::isMerged() const{
  return merged_;
}

// 存放表格區一列的資訊結構(定義)
void TableRow::addCell(TableCell cell){
  cells_.push_back(std::move(cell));
}
void TableRow::addCellx(int x){
  cellxNum_.push_back(x);
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

const std::vector<TableCell>& TableRow::cells() const {
  return cells_;
}
const std::vector<int>& TableRow::cellxs() const{
  return cellxNum_;
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

  i += first->text.size();

  TableBlock table;
  TableRow currentRow;
  std::string cellText;
  bool mergeStart = false;
  bool merged = false;

  for(;i<text.size();){
    auto marker = findMarker(text, i);

    if (!marker) {
      cellText.push_back(text[i]);
      ++i;
      continue;
    }

    switch(marker -> type){
      case MarkerType::TableCell : 
      {
        TableCell currentCell(std::move(cellText));

        currentCell.setMergeStart(mergeStart);
        currentCell.setMerged(merged);

        currentRow.addCell(std::move(currentCell));
        
        cellText.clear();

        mergeStart = false;
        merged = false;
        
        i += marker -> text.size();
        continue;
      } 
      case MarkerType::TableRow :
      {
        if(!cellText.empty()){
          TableCell currentCell(std::move(cellText));

          currentCell.setMergeStart(mergeStart);
          currentCell.setMerged(merged);

          currentRow.addCell(std::move(currentCell));
          
          cellText.clear();

          mergeStart = false;
          merged = false;
        }

        if(!currentRow.empty()){
          table.addRow(std::move(currentRow));
          currentRow = TableRow{};
        }
        
        i += marker -> text.size();

        auto next = findMarker(text,i);

        if(next &&
          (next->type == MarkerType::TableTrowd ||
           next->type == MarkerType::TableIntbl))
        {
          continue;
        }

        return TableBlockResult{std::move(table),i};
      }
      case MarkerType::TableTrowd:
      case MarkerType::TableIntbl:
      {
        i += marker -> text.size();
        continue;
      }
      case MarkerType::Line:
      case MarkerType::Par:
      {
        cellText.push_back('\n');
        i += marker->text.size();
        continue;
      }
      case MarkerType::LastRow:
      {
        currentRow.setLastRow();

        i += marker ->text.size();
        continue;
      }
      case MarkerType::NestCell:
      case MarkerType::NestRow:
      {
        // 第一版不支援巢狀表格先跳過
        i += marker ->text.size();
        continue;
      }
      case MarkerType::Trhdr:
      {
        currentRow.setHeaderRow();
        i += marker ->text.size();
        continue;
      }
      case MarkerType::Clmgf:
      {
        mergeStart = true;
        i += marker ->text.size();
        continue;
      }
      case MarkerType::Clmrg:
      {
        merged = true;
        i += marker ->text.size();
        continue;
      }
      case MarkerType::Cellx:
      {
        if(marker->value){
          currentRow.addCellx(*marker->value);
        }
        i += marker ->text.size();
        continue;
      }
      case MarkerType::TrLeft:
      {
        if(marker->value){
          currentRow.setTrleft(*marker->value);
        }
        i += marker ->text.size();
        continue;
      }
      case MarkerType::Itap:
      {
        if(marker->value){
          currentRow.setItap(*marker->value);
        }
        i += marker ->text.size();
        continue;
      }
      default :
      {
        if(!table.empty()){
          return TableBlockResult{std::move(table),i};
        }
        return std::nullopt;
      }
    }
  }

  if(!cellText.empty()){
    currentRow.addCell(TableCell(std::move(cellText)));
  }

  if(!currentRow.empty()){
    table.addRow(std::move(currentRow));
  }

  if(!table.empty()){
    return TableBlockResult{std::move(table), text.size()};
  }

  return std::nullopt;
}