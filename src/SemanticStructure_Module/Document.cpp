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
    Line
  };

  struct MarkerInfo{
    MarkerType type;
    std::string_view text;
  };

  constexpr std::array<MarkerInfo,7> markers{{
    {MarkerType::TableTrowd, "@@RTF_TABLE_TROWD@@"},
    {MarkerType::TableCell , "@@RTF_TABLE_CELL@@" },
    {MarkerType::TableRow  , "@@RTF_TABLE_ROW@@"  },
    {MarkerType::TableIntbl, "@@RTF_TABLE_INTBL@@"},
    {MarkerType::Image     , "@@RTF_IMAGE"       },
    {MarkerType::Par       , "@@par@@"            },
    {MarkerType::Line      , "@@line@@"           }
  }};

  std::optional<MarkerInfo> findMarker(std::string_view content, size_t pos) {
    for (const auto& marker : markers) {
        if (pos + marker.text.size() <= content.size() &&
            content.compare(pos, marker.text.size(), marker.text) == 0) {
            return marker;
        }
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
    constexpr std::string_view Mark = "@@";

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
          appendText(" | ");
          i += marker->text.size();
          break;

        case MarkerType::TableRow:
          //appendText("\n");
          parblock = nullptr;
          i += marker->text.size();
          break;

        case MarkerType::TableTrowd:
        case MarkerType::TableIntbl:
          appendText("| ");
          i += marker->text.size();
          break;
        
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
