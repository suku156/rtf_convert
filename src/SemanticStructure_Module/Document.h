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
#include<vector>
#include<memory>

// 所有節點共用的介面
class Node{
public:
  enum class Type{
    Text,
    Image
  };

  virtual ~Node() = default;
  virtual Type getType() const = 0;
};

// 文字節點
class TextNode : public Node{
  std::string text_;
public:
  explicit TextNode(std::string text) : text_(std::move(text)){}
  Type getType() const override;
  const std::string& text() const;
};

// 圖片節點
class ImageNode : public Node{
  std::string imageId_;
public:
  explicit ImageNode(std::string imageId) : imageId_(std::move(imageId)){}
  Type getType() const override;
  const std::string& imageId() const;
};

// 存放節點用的段落空間繼承之介面
class Block{
public:
  virtual ~Block() = default;
};

// 存放文字節點專用的段落空間
class ParagraphBlock : public Block{
  int identLevel_ = 0;
  std::vector<std::unique_ptr<TextNode>> texts_;
public:
  explicit ParagraphBlock(int indent =0) : identLevel_(indent) {}
  void addText(std::unique_ptr<TextNode> text);
  const std::vector<std::unique_ptr<TextNode>>& texts() const;
};

// 存放圖片節點專用的段落空間
class ImageBlock : public Block{
  std::unique_ptr<ImageNode> image_;
public:
  explicit ImageBlock(std::unique_ptr<ImageNode> img) : image_(std::move(img)) {}
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
  bool isImageline(const std::string& line);
  std::string extractImageId(const std::string& text);
};
