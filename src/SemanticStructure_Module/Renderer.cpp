// =====================================================
// Module : Renderer (implementation)
// =====================================================
#include "Renderer.h"
#include "SemanticStructure_Module/Document.h"
#include<ostream>
#include<string>

// Txt 類型的函式定義
void TxtRenderer::render(const Document& Doc , std::ostream& out){
    for(const auto& blk: Doc.blocks()){
      if(auto* p = dynamic_cast<const ParagraphBlock*>(blk.get())){
        renderParagraph(*p,out);
        out << "\n";
      }else if(auto* img = dynamic_cast<const ImageBlock*>(blk.get())){
        renderImage(*img,out);
        out << "\n";
      }
    }
}
void TxtRenderer::renderParagraph(const ParagraphBlock& p,std::ostream& out){
  for(const auto& text : p.texts()){
    out << text -> text();
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

// MD 類型的函式定義
void MarkdownRenderer::render(const Document& Doc , std::ostream& out){
    for(const auto& blk: Doc.blocks()){
      if(auto* p = dynamic_cast<const ParagraphBlock*>(blk.get())){
        renderParagraph(*p,out);
        out << "\n";
      }else if(auto* img = dynamic_cast<const ImageBlock*>(blk.get())){
        renderImage(*img,out);
        out << "\n";
      }
    }
}
void MarkdownRenderer::renderParagraph(const ParagraphBlock& p,std::ostream& out){
  for(const auto& text : p.texts()){
    out << text -> text();
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

// Html 類型的函式定義
void HtmlRenderer::render(const Document& Doc,std::ostream& out){
    out << "<!DOCTYPE html>\n";
    out << "<html>\n<head>\n";
    out << "<meta charset=\"utf-8\">\n";
    out << "<style>\n";
    out << ".rtf-paragraph {";
    out << " white-space: pre-wrap;\n";
    out << "}\n";
    out << "</style>\n";
    out << "</head>\n<body>\n";

    for(const auto& blk: Doc.blocks()){
      if(auto* p = dynamic_cast<const ParagraphBlock*>(blk.get())){
        renderParagraph(*p,out);
      }else if(auto* img = dynamic_cast<const ImageBlock*>(blk.get())){
        renderImage(*img,out);
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
    out << "<p class=\"rtf-paragraph\">";

    for(const auto& text : p.texts()){
      renderEscapedText(text ->text(),out);
    }
    out << "</p>\n";
}
void HtmlRenderer::renderImage(const ImageBlock& img,std::ostream& out){
    const auto& mark = img.image().imageMark();

    if(mark){
      out << "<div class=\"image-block\">\n";
      switch(mark->type){
        case ImageMarkerType::Normal :
        out << "img src=\"[IMAGE : " << mark->id << "]\" alt=\"\">\n";
        break;
        case ImageMarkerType::Error :
        out << "img src=\"[IMAGE_ERR :  " << mark->id << "]\" alt=\"\">\n";
        break;
      }
      out << "</div>\n";
    }
    
}