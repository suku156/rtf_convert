// =====================================================
// Module  : RtfPreProcessor
// Author  : suku156
// Purpose : 在解碼前預先清理掉會影響後續處裡之群組
// Layer   : RtfGroupProcessor
//
// Depend  :
//   
// Used by :
// RtfProcessor
//
// Notes :
//   專門負責在解碼動作前清理掉會影響到後續處裡的星號群組
// =====================================================
#pragma once
#include<string>
#include<string_view>
#include<optional>

class logSystem;

struct GroupHeadInfo{
  bool isStar = false;
  std::optional<std::string> name = std::nullopt;
};

enum class PreDecodeGroupAction {
  // 整個群組移除
  Drop,
  // 進入群組內部繼續檢查
  Recurse,
  // 保留全部群組
  Keep,
  // 需特殊處裡之群組
  Special
};

struct PreImageMarkerInfo{
  bool hasMarker = false;
  std::string marker;
};

class RtfPreProcessor{
  logSystem& logger_;
public:
  // 建構時綁定 log 方便記錄
  explicit RtfPreProcessor(logSystem& logger): logger_(logger){}
  std::string process(std::string_view rtf);
private:
  bool start_with(std::string_view s,std::string_view prefix);
  size_t findGroupEnd(std::string_view group,size_t pos);
  GroupHeadInfo parseGroupHead(std::string_view group);
  PreDecodeGroupAction getGroupAction(GroupHeadInfo info);
  std::string processGroup(std::string_view group);
  std::string processView(std::string_view src, size_t pos =0);
  PreImageMarkerInfo containsMarker(std::string_view currentGroup);
};