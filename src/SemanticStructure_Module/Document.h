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
};
