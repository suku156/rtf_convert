// =====================================================
// Module  : SheetProcessor
// Author  : suku156
// Purpose : 用來處理 rtf 內部得表格相關 
// Layer   : SheetProcessors
//
// Depend  :
//   
// Used by :
//   RTFProcessor   
//
// Notes :
//  　處理　ｒｔｆ　內的表格區塊
// =====================================================
#pragma once
#include <string>
#include <cstddef>

struct sheetScope{
  size_t sheetstart_;
  size_t sheetend_;
  std::string newsheetStr_;
};

class sheetProcessor{
public:
   void processor(std::string& rtfContent);
private:
   bool isControlEnd(const std::string& s,size_t pos);
};