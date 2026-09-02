// =====================================================
// Module  : RtfPreProcessor
// Author  : suku156
// Purpose : 在解碼前預先處理會影響後續解析的 RTF 群組，
//           必要時將需要保留的資訊轉換為標記符號，
//           並移除或簡化不需要的群組內容。
// Layer   : RtfGroupProcessor
//
// Depend  :
//   
// Used by :
// RtfProcessor
//
// Notes :
//        專門負責在解碼前處理特殊 RTF 群組，
//        以避免其結構影響後續解析流程。
//        對於需要保留語意但無法直接進入後續流程的資訊，
//        可先轉換為 marker 延後處理。
//
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
  PreImageMarkerInfo containsImageMarker(std::string_view currentGroup);
  std::optional<std::string> extractPreservedContent(std::string_view group);
  std::optional<std::string> containsTableMarker(std::string_view group);
  std::optional<std::string> extractShapeTextMarker(std::string_view group);
  std::string processShapeTextRtf(std::string_view group);
  std::optional<std::string> extractShapeProperties(std::string_view group);
};