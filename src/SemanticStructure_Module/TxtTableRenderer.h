// =====================================================
// Module  : TxtTableRenderer
// Author  : suku156
// Purpose : 解讀語意結構中的表格區塊
// Layer   : SemanticStructure
//
// Depend  :
//   Document
//  
// Used by :
//   Renderer
//
// Notes :
//    負責 txt 版本的語意結構表格區塊處理
// =====================================================

#pragma once
#include <iosfwd>

class TableBlock;

namespace txt_table {

  void renderTable(const TableBlock& table, std::ostream& out);
}