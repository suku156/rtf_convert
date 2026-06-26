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

// 存放表格區類似一格(最小結構單位)的資訊結構
class TableCell{
  std::string text_;
  bool mergeStart_ = false;
  bool merged_ = false;
public :
  explicit TableCell(std::string text = {}) : text_(std::move(text)){}
  void setMergeStart(bool value = true);
  void setMerged(bool value = true);

  bool isMergeStart() const;
  bool isMerged() const;
  
  const std::string& text() const;
  bool empty() const;
};

// 存放表格區一列的資訊結構
class TableRow{
  int trLeft_ = 0;
  int itap_ = 1;
  bool lastRow_ = false;
  bool headerRow_ = false;
  std::vector<TableCell> cells_;
  std::vector<int> cellxNum_;
public:
  void addCell(TableCell cell);
  void addCellx(int x);
  void setTrleft(int left);
  void setLastRow(bool value = true);
  void setHeaderRow(bool value = true);
  void setItap(int value);
  
  const std::vector<TableCell>& cells() const;
  const std::vector<int>& cellxs() const;
  int trLeft() const;
  bool isLastRow() const;
  bool isHeaderRow() const;
  int Itap() const;

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
  bool empty() const;
};

// 用於回傳 表格段落處裡回傳用的容器
struct TableBlockResult{
  TableBlock table;
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
};
