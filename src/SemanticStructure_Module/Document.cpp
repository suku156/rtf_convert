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
const std::optional<ImageMarkerInfo>& ImageNode::imageMark() const{
  return imageMark_;
}

// 存放文字節點專用的段落空間函式定義
void ParagraphBlock::addText(std::unique_ptr<TextNode> text){
    texts_.push_back(std::move(text));
}
const std::vector<std::unique_ptr<TextNode>>& ParagraphBlock::texts() const{
    return texts_;
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
      
      std::string text = line;
      
      if(text.empty()){
        parblock = nullptr;
        continue;
      }else if(isImageline(text)){
        parblock = nullptr; // 確保遇到圖片標記符時一定會切換段落

        std::optional<ImageMarkerInfo> mark = extractImageId(text);// 擷取標記符數字

        doc.addBlock<ImageBlock>(std::make_unique<ImageNode>(mark));
        continue;
      }else{
        parblock = &doc.addBlock<ParagraphBlock>(0);
        parblock -> addText(std::make_unique<TextNode>(text));
      }
    }

    return doc;
}

bool DocumentBuilder::isImageline(const std::string& line){
    constexpr std::string_view imgHead = "@@RTF_IMAGE";
    constexpr std::string_view imgErr  = "_ERR_";
    
    if(line.size() < imgHead.size()) return false;
    if(line.compare(0,imgHead.size(),imgHead) != 0) return false;

    std::size_t pos = imgHead.size();

    if(pos >= line.size()) return false;
    
    if(pos + imgErr.size() < line.size() &&  
             line.compare(pos,imgErr.size(),imgErr) == 0){
      pos += imgErr.size();
    }else if(line[pos] == '_')
    {
      pos++;
    }else{
      return false;
    }

    if(pos >= line.size() ||
       !std::isdigit(static_cast<unsigned char>(line[pos]))){
        return false;
    }

    while(pos < line.size() &&
          std::isdigit(static_cast<unsigned char>(line[pos]))){
        ++pos;
    }
    
    return pos + 2 <= line.size() && line[pos] == '@' && line[pos + 1] == '@';
}
std::optional<ImageMarkerInfo> DocumentBuilder::extractImageId(const std::string& text){
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