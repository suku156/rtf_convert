// =====================================================
// Module  : RtfTablePreProcessor
// Author  : suku156
// Purpose : 用來處理 rtf 內部得表格相關 
// Layer   : RtfTableProcessor
//
// Depend  :
//   FieldGroupProcessor
//
// Used by :
//   RTFProcessor   
//
// Notes :
//  　預先處理　ｒｔｆ　內的表格區塊
//    將重要的控制符替換成標記符以便後續處裡
// =====================================================
#pragma once
#include <string>
#include <string_view>
#include <optional>
#include <cstddef>
#include <vector>
#include "LogSystem_Module/LogSystem.h"

enum class TableTokenType{
   text,
   cell,
   row
};

struct TableToken{
   TableTokenType type = TableTokenType::text;
   std::string text;
   bool hasRow = false;
   bool hasCell = false;
};

struct sheetScope{
  size_t sheetstart_;
  size_t sheetend_;
  std::string newsheetStr_;
};

struct TableStartMatch{
   size_t targetSize = 0;
   std::string_view marker;
};

struct NestTablePropsResult {
   std::string text;
   size_t nextPos = 0;
};




class sheetProcessor{
public:
   void processor(std::string& rtfContent,logSystem& logger);
private:
   bool isControlEnd(const std::string& s,size_t pos);
   bool isControlEnd(std::string_view s,size_t pos);
   TableToken groupProcessor(std::string_view groupView,logSystem& logger);
   size_t findNextTableEntry(const std::string& rtfContent,size_t from);
   bool start_with(std::string_view s,std::string_view prefix);
   std::optional<TableStartMatch> matchTableStart(std::string_view s, size_t i);
   size_t findGroupEnd(std::string_view group,size_t pos);
   void buildGroupStackUntil(std::string_view group,
                             std::vector<size_t>& groupStack,
                             size_t start ,size_t end);
   std::string preprocessNestedTableGroups(std::string_view group);
   bool isNestTablePropsStart(std::string_view group,size_t pos);
   std::optional<NestTablePropsResult> processNestTableProps(std::string_view group,size_t pos);
   std::string removeTableShapes(std::string_view group);
};