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
    out << std::string(p.indentLevel() *2 , ' ');
    
    for(const auto& text : p.texts()){
      out << text -> text();
    }
}
void TxtRenderer::renderImage(const ImageBlock& img,std::ostream& out){
    out << "[IMAGE: " << img.image().imageId() << "]";
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
    out << std::string(p.indentLevel() *2 , ' ');

    for(const auto& text : p.texts()){
      out << text -> text();
    }
}
void MarkdownRenderer::renderImage(const ImageBlock& img,std::ostream& out){
    out << "![](IMG_" << img.image().imageId() << ".png)";
}

// Html 類型的函式定義
void HtmlRenderer::render(const Document& Doc,std::ostream& out){
    out << "<!DOCTYPE html>\n";
    out << "<html>\n<head>\n";
    out << "<meta charset=\"utf-8\">\n";
    out << "<style>\n";
    out << ".indent-0 { margin-left: 0em; }\n";
    out << ".indent-1 { margin-left: 1em; }\n";
    out << ".indent-2 { margin-left: 2em; }\n";
    out << ".indent-3 { margin-left: 3em; }\n";
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
    out << "<p class=\"indent-" << p.indentLevel() << "\">";

    for(const auto& text : p.texts()){
      renderEscapedText(text ->text(),out);
    }
    out << "</p>\n";
}
void HtmlRenderer::renderImage(const ImageBlock& img,std::ostream& out){
    out << "<div class=\"image-block\">\n";
    out << "img src=\"IMG_" << img.image().imageId() << ".png\" alt=\"\">\n";
    out << "</div>\n";
}