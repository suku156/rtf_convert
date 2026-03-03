#pragma once
#include<ostream>
#include<string>

// forward declaration
class Document;
class ParagraphBlock;
class ImageBlock;

// 讀取語意結構用的類別 txt 用
class TxtRenderer{
public:
  void render(const Document& Doc , std::ostream& out);
private:
  void renderParagraph(const ParagraphBlock& p,std::ostream& out);
  void renderImage(const ImageBlock& img,std::ostream& out);
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