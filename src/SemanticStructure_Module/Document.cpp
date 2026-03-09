// =====================================================
// Module : Document (implementation)
// =====================================================
#include "Document.h"
#include<string>
#include<vector>
#include<memory>
#include<sstream>

// 文字節點函式定義
Node::Type TextNode::getType() const{
  return Type::Text;
}
const std::string& TextNode::text() const{
    return text_;
}

// 圖片節點函式定義
Node::Type ImageNode::getType() const{
  return Type::Image;
}
const std::string& ImageNode::imageId() const{
  return imageId_;
}

// 存放文字節點專用的段落空間函式定義
void ParagraphBlock::addText(std::unique_ptr<TextNode> text){
    texts_.push_back(std::move(text));
}
const std::vector<std::unique_ptr<TextNode>>& ParagraphBlock::texts() const{
    return texts_;
}
int ParagraphBlock::indentLevel() const{
    return identLevel_;
}

// 存放圖片節點專用的段落空間函式定義
const ImageNode& ImageBlock::image() const{
    return *image_;
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
    std::istringstream isss(content);
    std::string line;
    Document doc;
    ParagraphBlock* parblock = nullptr;
    

    while(std::getline(isss,line)){
      
      std::string text = trimLeft(line);
      
      if(text.empty()){
        parblock = nullptr;
        continue;
      }else if(isImageline(text)){
        parblock = nullptr; // 確保遇到圖片標記符時一定會切換段落

        std::string imgid = extractImageId(text);// 擷取標記符數字

        doc.addBlock<ImageBlock>(std::make_unique<ImageNode>(imgid));
        continue;
      }else{
        int indent = calcIndentLevel(line);

        if(!parblock || parblock ->indentLevel() != indent){ // 沒有指向段落 或 前方空格階層不同時 指向新段落
          parblock = &doc.addBlock<ParagraphBlock>(indent);
        }

        parblock -> addText(std::make_unique<TextNode>(text));
      }
    }

    return doc;
}
int DocumentBuilder::calcIndentLevel(const std::string& line){
    int space = 0;
    for(char c : line){
      if(c == ' '){
        space++;
      }else{
        break;
      }
    }
    return space / 2;
}
std::string DocumentBuilder::trimLeft(const std::string& line){
    std::size_t i = 0; 
    while(i<line.size() && (line[i] == ' ' || line[i] == '\t')){
      i++;
    }
    return line.substr(i);
}
bool DocumentBuilder::isImageline(const std::string& line){
    if(line.size() < 8) return false;
    if(line.rfind("[[IMG_",0) != 0) return false;

    std::size_t pos = 6;
    if(pos >= line.size() || !isdigit(static_cast<unsigned char>(line[pos]))){
      return false;
    }

    while(pos < line.size() && isdigit(static_cast<unsigned char>(line[pos]))) pos++;

    return (pos + 1 == line.size() -1 && line[pos] == ']' && line[pos + 1] == ']');
}
std::string DocumentBuilder::extractImageId(const std::string& text){
    std::size_t pos = 6;
    std::size_t end = pos;
    while(end < text.size() && isdigit(static_cast<unsigned char>(text[end]))) end++;

    return text.substr(pos,end - pos);
}