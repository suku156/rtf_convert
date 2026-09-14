// =====================================================
// Module : Renderer (implementation)
// =====================================================
#include "Renderer.h"
#include "TxtTableRenderer.h"
#include "MdTableRenderer.h"
#include "HtmlTableRenderer.h"
#include "SemanticStructure_Module/Document.h"
#include<ostream>
#include<string>
#include <utility>
#include <iostream>

namespace{

  // 給文字圖插入指令函式用的結構
  struct lineSwitch{
    bool hasBoxDrawing = false;
    bool emitted = false;
    std::string line;
  };

  // 文字圖常出現的特殊符號檢查
  // 逐一讀取 UTF-8 code point，遇到 Box Drawing 就回傳 true
  bool containsBoxDrawing(std::string_view text){
    size_t i = 0;

    while (i < text.size()) {
      unsigned char c = static_cast<unsigned char>(text[i]);
      char32_t cp = 0;

      if (c < 0x80) {                    // ASCII
        cp = c;
        ++i;
      }
      else if ((c & 0xE0) == 0xC0 && i + 1 < text.size()) {
          cp = ((c & 0x1F) << 6) |
               (static_cast<unsigned char>(text[i + 1]) & 0x3F);
          i += 2;
      }
      else if ((c & 0xF0) == 0xE0 && i + 2 < text.size()) {
          cp = ((c & 0x0F) << 12) |
               ((static_cast<unsigned char>(text[i + 1]) & 0x3F) << 6) |
               (static_cast<unsigned char>(text[i + 2]) & 0x3F);
          i += 3;
      }
      else if ((c & 0xF8) == 0xF0 && i + 3 < text.size()) {
        cp = ((c & 0x07) << 18) |
             ((static_cast<unsigned char>(text[i + 1]) & 0x3F) << 12) |
             ((static_cast<unsigned char>(text[i + 2]) & 0x3F) << 6) |
             (static_cast<unsigned char>(text[i + 3]) & 0x3F);
        i += 4;
      }
      else { // 非法 UTF-8
        ++i;
         continue;
      }

      if (cp >= 0x2500 && cp <= 0x257F) {
        return true;
      }
    }

    return false;
  }

  // 用來初步判斷一個文字空間內是否有文字圖
  bool containsTextDiagram(const ParagraphBlock& p){
    constexpr size_t minBoxDrawingLines = 3;
    
    size_t boxDrawingLines = 0;

    // 以 \n 為依據取出 std::string 中的文字在此定義為一行
    for(const auto& text : p.texts()){
      std::string line;

      for(size_t i = 0;i < text.text().size();i++){
        const char c = text.text()[i];
        
        if(c == '\n'){
          
          if(containsBoxDrawing(line)){
            ++boxDrawingLines;

            if(boxDrawingLines >= minBoxDrawingLines){
              return true;
            }
          }
            
          line.clear();
          continue;
        }

        line.push_back(c);
      }

      // 檢查最後一行
      if(!line.empty() && containsBoxDrawing(line)){
        ++boxDrawingLines;

        if(boxDrawingLines >= minBoxDrawingLines){
          return true;
        }
      }
    }

    return false;
  }

  // 文字圖區塊開始之判斷條件
  bool isTextPictStart(const lineSwitch& line0,
    const lineSwitch& line1,
    const lineSwitch& line2)
  {
    // Box + Box
    if(line0.hasBoxDrawing && line1.hasBoxDrawing){
      return true;
    }

    // Box + 空行 + Box
    if(line0.hasBoxDrawing &&
        line1.line.empty() &&
        line2.hasBoxDrawing)
    {
      return true;
    }

    return false;
  }
}




// Txt 類型的函式定義
void TxtRenderer::render(const Document& Doc , std::ostream& out){
  const auto& blocks = Doc.blocks();

  for(size_t i = 0; i < blocks.size(); ++i){
    const auto& blk = blocks[i];
    
    if(auto* p = dynamic_cast<const ParagraphBlock*>(blk.get())){
      renderParagraph(*p,out);
      out << "\n";
    }else if(auto* img = dynamic_cast<const ImageBlock*>(blk.get())){
      renderImage(*img,out);
      out << "\n";
    }else if(auto* table = dynamic_cast<const TableBlock*>(blk.get())){
      renderTable(*table, out);
      out << "\n";
    }
  }
}
void TxtRenderer::renderParagraph(const ParagraphBlock& p,std::ostream& out){
  for(const auto& text : p.texts()){
    out << text.text() ;
  }
}
void TxtRenderer::renderImage(const ImageBlock& img,std::ostream& out){
  const auto& mark = img.image().imageMark();

  if(mark){
    switch(mark->type){
      case ImageMarkerType::Normal :
      out << "[IMAGE : " << mark->id << "]";
      break;
      case ImageMarkerType::Error :
      out << "[IMAGE_ERR : " << mark->id << "]";
      break;
    }
  } 
}
void TxtRenderer::renderTable(const TableBlock& table,std::ostream& out){
  txt_table::renderTable(table,out);
}

// MD 類型的函式定義
void MarkdownRenderer::render(const Document& Doc , std::ostream& out){
  const auto& blocks = Doc.blocks();

  for(size_t i = 0; i < blocks.size(); ++i){
    const auto& blk = blocks[i];

    if(auto* p = dynamic_cast<const ParagraphBlock*>(blk.get())){
      renderParagraph(*p,out);
      out << "\n";
    }else if(auto* img = dynamic_cast<const ImageBlock*>(blk.get())){
      renderImage(*img,out);
      out << "\n";
    }else if(auto* table = dynamic_cast<const TableBlock*>(blk.get())){
      renderTable(*table, out);
      out << "\n";
    }
  }
}
void MarkdownRenderer::renderParagraph(const ParagraphBlock& p,std::ostream& out){
  
  if(containsTextDiagram(p)){
    renderParagraphWithTextPict(p,out);
    return;
  }
  
  for(const auto& text : p.texts()){
    out << text.text();
  }
}
void MarkdownRenderer::renderImage(const ImageBlock& img,std::ostream& out){
  const auto& mark = img.image().imageMark();

  if(mark){
    switch(mark->type){
      case ImageMarkerType::Normal :
      out << "[IMAGE : " << mark->id << "]";
      break;
      case ImageMarkerType::Error :
      out << "[IMAGE_ERR : " << mark->id << "]";
      break;
    }
  } 
}
void MarkdownRenderer::renderTable(const TableBlock& table,std::ostream& out){
  md_table::renderTable(table,out);
}
void MarkdownRenderer::renderParagraphWithTextPict
(const ParagraphBlock& p,std::ostream& out)
{
  lineSwitch lineSwitch_0;
  lineSwitch lineSwitch_1;
  lineSwitch lineSwitch_2;

  bool inTextPict = false;
  size_t noBoxDrawingCount = 0;

  auto pushLine = [&](std::string line)
  {
    lineSwitch_0 = std::move(lineSwitch_1);
    lineSwitch_1 = std::move(lineSwitch_2);

    lineSwitch_2.line = std::move(line);
    lineSwitch_2.hasBoxDrawing = containsBoxDrawing(lineSwitch_2.line);
    lineSwitch_2.emitted = false;
  };

  auto writeLineSwitch = [&out](lineSwitch& line){
    if(line.emitted) return;
    
    line.emitted = true;
    out << line.line;
  };

  for(const auto& text : p.texts()){

    std::string currentLine;

    for(size_t i = 0; i < text.text().size(); ++i){

      const char c = text.text()[i];

      if(c == '\n'){

        pushLine(std::move(currentLine));
        currentLine.clear();

        // 尚未進入文字圖
        if(!inTextPict){
          
          // 文字圖開始判斷之條件
          if(isTextPictStart(lineSwitch_0,
                            lineSwitch_1,
                            lineSwitch_2))
          {
            inTextPict = true;

            out << "\n```text\n";

            writeLineSwitch(lineSwitch_0);
            writeLineSwitch(lineSwitch_1);
            writeLineSwitch(lineSwitch_2);
            
            noBoxDrawingCount = 0;
          }
          else{
            // lineSwitch_0 已經確定不是文字圖開始點，
            // 可以正常輸出
            if(!lineSwitch_0.line.empty()){
              writeLineSwitch(lineSwitch_0);
            }
          }
        }
        else{
          // 已經在文字圖中
          // 判斷是否有文字圖符號
          if(lineSwitch_2.line.empty()){
            // 空行：不影響文字圖狀態
          }
          else if(lineSwitch_2.hasBoxDrawing){
            noBoxDrawingCount = 0;
          }
          else{
            ++noBoxDrawingCount;
          }

          // 這裡暫時簡單處理：
          if(noBoxDrawingCount >= 2){
            out << "\n```\n";
            inTextPict = false;
          }else{
            writeLineSwitch(lineSwitch_2);
          }
        }

        continue;
      }

      currentLine.push_back(c);
    }

    // 處理最後沒有 \n 的一行
    if(!currentLine.empty()){

      pushLine(std::move(currentLine));

      writeLineSwitch(lineSwitch_2);
      
    }
  }

  writeLineSwitch(lineSwitch_0);
  writeLineSwitch(lineSwitch_1);
  writeLineSwitch(lineSwitch_2);

  if(inTextPict){
    out << "\n```\n";
  }
}


// Html 類型的函式定義
void HtmlRenderer::render(const Document& Doc,std::ostream& out){
  out << "<!DOCTYPE html>\n";
  out << "<html>\n<head>\n";
  out << "<meta charset=\"utf-8\">\n";
  out << "<style>\n";
  
  out << ".rtf-paragraph {";
  out << " white-space: pre-wrap;\n";
  out << "}\n";

  out << ".rtf-text-diagram {";
  out << " font-family: monospace;\n";
  out << "}\n";

  out << ".rtf-table {\n";
  out << "  border-collapse: collapse;\n";
  out << "}\n";

  out << ".rtf-table td {\n";
  out << "  border: 1px solid black;\n";
  out << "  padding: 4px;\n";
  out << "  height: 24pt;\n";
  out << "  vertical-align: top;\n";
  out << "  word-break: break-word;\n";
  out << "}\n";

  out << "</style>\n";
  out << "</head>\n<body>\n";

  const auto& blocks = Doc.blocks();

  for(size_t i = 0; i < blocks.size(); ++i){
    
    const auto& blk = blocks[i];

    if(auto* p = dynamic_cast<const ParagraphBlock*>(blk.get())){
      renderParagraph(*p,out);
    }else if(auto* img = dynamic_cast<const ImageBlock*>(blk.get())){
      renderImage(*img,out);
    }else if(auto* table = dynamic_cast<const TableBlock*>(blk.get())){
      renderTable(*table, out);
      out << "\n";
    }
  }

  out << "</body>\n</html>\n";
}
void HtmlRenderer::renderEscapedText(const std::string& text,std::ostream& out){
    for(unsigned char c : text){
      switch(c){
        case '&' : out << "&amp;" ; break;
        case '<' : out << "&lt;"  ; break;
        case '>' : out << "&gt;"  ; break;
        case '"' : out << "&quot;"; break;
        default : out << c;
      }
    }
}
void HtmlRenderer::renderParagraph(const ParagraphBlock& p,std::ostream& out){
  if(containsTextDiagram(p)){
    renderParagraphWithTextPict(p,out);
    return;
  }
  
  out << "<p class=\"rtf-paragraph\">";

  for(const auto& text : p.texts()){
    
    renderEscapedText(text.text(),out);
  }
  
  out << "</p>\n";
}
void HtmlRenderer::renderImage(const ImageBlock& img,std::ostream& out){
    const auto& mark = img.image().imageMark();

    if(mark){
      out << "<div class=\"image-block\">\n";
      switch(mark->type){
        case ImageMarkerType::Normal :
        out << "[IMAGE : " << mark->id << "]\n";
        break;
        case ImageMarkerType::Error :
        out << "[IMAGE_ERR :  " << mark->id << "]\n";
        break;
      }
      out << "</div>\n";
    }
    
}
void HtmlRenderer::renderTable(const TableBlock& table,std::ostream& out){
  html_table::renderTable(table,out);
}

void HtmlRenderer::renderParagraphWithTextPict(const ParagraphBlock& p,std::ostream& out){
  
  out << "<p class=\"rtf-paragraph\">";

  lineSwitch lineSwitch_0;
  lineSwitch lineSwitch_1;
  lineSwitch lineSwitch_2;

  bool inTextPict = false;
  size_t noBoxDrawingCount = 0;

  auto pushLine = [&](std::string line)
  {
    lineSwitch_0 = std::move(lineSwitch_1);
    lineSwitch_1 = std::move(lineSwitch_2);

    lineSwitch_2.line = std::move(line);
    lineSwitch_2.hasBoxDrawing = containsBoxDrawing(lineSwitch_2.line);
    lineSwitch_2.emitted = false;
  };

  auto writeLineSwitch = [&out,this](lineSwitch& line){
    if(line.emitted) return;
    
    line.emitted = true;
    this->renderEscapedText(line.line, out);
    out << '\n';
  };

  for(const auto& text : p.texts()){

    std::string currentLine;

    for(size_t i = 0; i < text.text().size(); ++i){

      const char c = text.text()[i];

      if(c == '\n'){

        pushLine(std::move(currentLine));
        currentLine.clear();

        // 尚未進入文字圖
        if(!inTextPict){
          
          // 文字圖開始判斷之條件
          if(isTextPictStart(lineSwitch_0,
                            lineSwitch_1,
                            lineSwitch_2))
          {
            inTextPict = true;

            out << "<span class=\"rtf-text-diagram\">";

            writeLineSwitch(lineSwitch_0);
            writeLineSwitch(lineSwitch_1);
            writeLineSwitch(lineSwitch_2);

            noBoxDrawingCount = 0;
          }
          else{
            // lineSwitch_0 已經確定不是文字圖開始點，
            // 可以正常輸出
            if(!lineSwitch_0.line.empty()){
              writeLineSwitch(lineSwitch_0);
            }
          }
        }
        else{
          // 已經在文字圖中
          // 判斷是否有文字圖符號
          if(lineSwitch_2.line.empty()){
            // 空行：不影響文字圖狀態
          }
          else if(lineSwitch_2.hasBoxDrawing){
            noBoxDrawingCount = 0;
          }
          else{
            ++noBoxDrawingCount;
          }

          // 這裡暫時簡單處理：
          if(noBoxDrawingCount >= 2){
            out << "</span>";
            inTextPict = false;
          }else{
            writeLineSwitch(lineSwitch_2);
          }
        }

        continue;
      }

      currentLine.push_back(c);
    }

    // 處理最後沒有 \n 的一行
    if(!currentLine.empty()){

      pushLine(std::move(currentLine));

      writeLineSwitch(lineSwitch_2);
      
    }
  }

  writeLineSwitch(lineSwitch_0);
  writeLineSwitch(lineSwitch_1);
  writeLineSwitch(lineSwitch_2);
  
  if(inTextPict){
    out << "</span>";
  }

  out << "</p>\n";
}