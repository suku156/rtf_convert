// =====================================================
// Module  : StartDestinationProcessor
// Author  : suku156
// Purpose : 在找到相關群組時專門負責處裡該群組
// Layer   : RtfGroupProcessor
//
// Depend  :
//   
// Used by :
//   textProcessor   
//
// Notes :
//   專門負責處裡在其餘流程中找到的 {\*\ 開頭之特別群組
//   不負責判斷 {\*\...} 群組之結構 只用來 判斷 + 分析 內容
// =====================================================
#pragma once
#include <string>
#include <string_view>
#include <optional>

struct StarGroupResult{
  std::optional<std::string> text = std::nullopt;
  bool unknown = false;
  std::string unknownGroupName;
};

class StartGroupProcessor {
public:
  StarGroupResult groupProcessor(std::string_view group);
private:
  bool start_with(std::string_view s,std::string_view prefix);
  bool isKnownDroppableGroup(std::string_view g);
  bool isKnownTextCarrierGroup(std::string_view g);
  std::string extractVisibleText(std::string_view g);
  std::string findStartGroupName(std::string_view g);
};