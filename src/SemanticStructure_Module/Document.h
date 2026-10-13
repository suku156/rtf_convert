// =====================================================
// Module  : Document
// Author  : suku156
// Purpose : 語意結構建立
// Layer   : SemanticStructure
//
// Depend  :
//
// Used by :
//   RTFProcessor   
//   Renderer
//
// Notes :
//   用來存放語意結構的架構,還有將處裡好的字串拆解成語意結構的函式 
// =====================================================
#pragma once
#include<string>
#include<string_view>
#include<vector>
#include<memory>
#include<optional>

//提前宣告區
class Block;
class TableBlock;

enum class ImageMarkerType{
    Normal,
    Error
};

struct ImageMarkerInfo{
    ImageMarkerType type;
    std::string id;
};


// 文字節點
class TextNode{
  std::string text_;
public:
  explicit TextNode(std::string text) : text_(std::move(text)){}
  const std::string& text() const;
};

// 圖片節點
class ImageNode {
  std::optional<ImageMarkerInfo> imageMark_;
public:
  explicit ImageNode(std::optional<ImageMarkerInfo> imageMark) : imageMark_(std::move(imageMark)){}
  const std::optional<ImageMarkerInfo>& imageMark() const;
};

// 用來表示 table 內部 表格與文字的順序性用的 enum
enum class NestedTablePlacement {
  BeforeText,
  AfterText,
  None
};

// 存放表格區類似一格(最小結構單位)的資訊結構
// 因為有多放  std::unique_ptr<TableBlock> 的關係此類別禁止複製
class TableCell{
  std::string text_;
  std::unique_ptr<TableBlock> nestedTable_;

  NestedTablePlacement nestedTablePlacement_ = NestedTablePlacement::None;
public :
  explicit TableCell(std::string text = {}) : text_(std::move(text)){}
 
  // 移動語意用預設
  TableCell(TableCell&&) noexcept = default;
  TableCell& operator=(TableCell&&) noexcept = default;
  // 禁止複製
  TableCell(const TableCell&) = delete;
  TableCell& operator=(const TableCell&) = delete;

  bool hasNestedTable() const;
  const TableBlock* nestedTable() const;
  void setNestedTable(std::unique_ptr<TableBlock> table);
  const std::string& text() const;
  bool empty() const;
  NestedTablePlacement nestedTablePlacement() const;
  void setNestedTablePlacement(NestedTablePlacement placement);
};

// 用來存放與 cell 有關的資訊 cellx ,垂直合併,水平合併等狀態
struct TableCellDef{
  int cellx = 0;

  bool horizontalMergeStart = false;
  bool horizontalMerged = false;
  bool verticalMergeStart = false;
  bool verticalMerged = false;
};

// 存放表格區一列的資訊結構
class TableRow{
  int trLeft_ = 0;
  int itap_ = 1;
  int trrh_ = 0;
  bool lastRow_ = false;
  bool headerRow_ = false;
  bool mergedRow_ = false;
  
  std::vector<TableCell> cells_;
  std::vector<TableCellDef> cellDefs_;
public:
  void addCell(TableCell cell);
  void addCellDef(TableCellDef def);
  void setTrleft(int left);
  void setLastRow(bool value = true);
  void setHeaderRow(bool value = true);
  void setItap(int value);
  void setTrrh(int value);
  void setMergedRow(bool value = true);
  void changeCellDefsCellx(const std::vector<int>& target);
  
  const std::vector<TableCell>& cells() const;
  const std::vector<TableCellDef>& cellDefs() const;
  int trLeft() const;
  bool isLastRow() const;
  bool isHeaderRow() const;
  bool isMergedRow() const;
  int Itap() const;
  int trrh() const;

  bool empty() const;
};


// 存放節點用的段落空間繼承之介面
class Block{
public:
  virtual ~Block() = default;
};

// 存放文字節點專用的段落空間
class ParagraphBlock : public Block{
  int identLevel_ = 0;
  std::vector<TextNode> texts_;
public:
  explicit ParagraphBlock(int indent =0) : identLevel_(indent) {}
  void addText(TextNode text);
  const std::vector<TextNode>& texts() const;
};

// 存放圖片節點專用的段落空間
class ImageBlock : public Block{
  ImageNode image_;
public:
  explicit ImageBlock(ImageNode img) : image_(std::move(img)) {}
  const ImageNode& image() const;
};


// 存放表格專用的段落空間
class TableBlock : public Block{
 std::vector<TableRow> rows_;
public:
  void addRow(TableRow row);
  const std::vector<TableRow>& rows() const;
  void changeMergedRow(int Trleft,const std::vector<int>& cellxRow);
  void appendRows(TableBlock&& other);
  bool hasMergedRow() const;
  bool empty() const;
};

// 用於回傳 表格段落處裡回傳用的容器
struct TableBlockResult{
  TableBlock table;
  size_t nextPos = 0 ;
};

// 存放圖形群組(非圖片)專用的段落空間
// 目前只支援 純文字註解
class ShapeTextBlock : public Block {
  std::string text_;

  int top_ = 0;
  int bottom_ = 0;
  int left_ = 0;
  int right_ = 0;
  int zOrder_ = 0;

public:
  ShapeTextBlock(std::string text,
                 int top,
                 int bottom,
                 int left,
                 int right,
                 int zOrder)
    : text_(std::move(text)),
      top_(top),
      bottom_(bottom),
      left_(left),
      right_(right),
      zOrder_(zOrder)
  {}

  const std::string& text() const;
  int top() const;
  int bottom() const;
  int left() const;
  int right() const;
  int zOrder() const;
};

// 用於回傳 圖形段落處裡回傳用的容器
struct ShapeTextBlockResult{
  ShapeTextBlock shape;
  size_t nextPos = 0 ;
};


// 用來存放各種段落空間的容器
class Document{
  std::vector<std::unique_ptr<Block>> blocks_;
public:
  //新增任意一種 block
  template<typename BlockT,typename... Args>
  BlockT& addBlock(Args&&... args);
  const std::vector<std::unique_ptr<Block>>& blocks() const;
};

// 將處理好的字串拆解成語意結構
class DocumentBuilder{
public:
  Document build(const std::string& content);
private:
  std::optional<ImageMarkerInfo> extractImageId(std::string_view text);
  std::optional<TableBlockResult> parseTableBlock(std::string_view text,size_t pos);
  std::optional<ShapeTextBlockResult> parseShapeBlock(std::string_view text,size_t pos);
  bool canMergeTable(const TableBlock& pendingTable ,const TableBlock& resultTable);
  void revisionMergeTable(TableBlock& pendingTable);
};
