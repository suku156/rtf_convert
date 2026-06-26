// =====================================================
// Module  : Renderer
// Author  : suku156
// Purpose : 解讀語意結構
// Layer   : SemanticStructure
//
// Depend  :
//   Document
//  
// Used by :
//   RTFProcessor   
//
// Notes :
//   可以依照指定格式解讀語意結構成為目標格式
//   目前支援 : txt,md,html 
// =====================================================
#pragma once
#include<ostream>
#include<string>

// forward declaration
class Document;
class ParagraphBlock;
class ImageBlock;
class TableBlock;

// 讀取語意結構用的類別 txt 用
class TxtRenderer{
public:
  void render(const Document& Doc , std::ostream& out);
private:
  void renderParagraph(const ParagraphBlock& p,std::ostream& out);
  void renderImage(const ImageBlock& img,std::ostream& out);
  void renderTable(const TableBlock& table,std::ostream& out);
};

// 讀取語意結構用的類別 md 用
class MarkdownRenderer{
public:
  void render(const Document& Doc , std::ostream& out);
private:
  void renderParagraph(const ParagraphBlock& p,std::ostream& out);
  void renderImage(const ImageBlock& img,std::ostream& out);
};

// 讀取語意結構用的類別 Html 用
class HtmlRenderer{
public:
  void render(const Document& Doc,std::ostream& out);
private:
  void renderEscapedText(const std::string& text,std::ostream& out);
  void renderParagraph(const ParagraphBlock& p,std::ostream& out);
  void renderImage(const ImageBlock& img,std::ostream& out); 
};