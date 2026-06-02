// =====================================================
// Module  : fieldGroupProcessor
// Author  : suku156
// Purpose : 在找到相關群組時專門負責處裡該群組
// Layer   : RtfGroupProcessor
//
// Depend  :
//   
// Used by :
//   textProcessor   
//   sheetProcessor
//
// Notes :
//   專門負責處裡在其餘流程中找到的 {\field 群組
// =====================================================

#pragma once
#include <string>
#include <string_view>
#include <optional>
#include <cstddef>
#include <vector>

enum class fieldKind{
  Unknown,
  Shape,
  Hyperlink,
  Page,
  Ref
};

struct FieldResultGroup{
  size_t start = 0;
  // 為群組結尾後面一格
  size_t end = 0;
  // 用來表示有無處理過
  //bool processed = false;
  // 用來表示是否有找到正文還是沒有正文
  //bool hasVisibleText = false;
  std::string completedStr{};
};

class fieldGroupProcessor{
public:
  std::optional<std::string> groupProcessor(std::string_view group);
private:
  bool start_with(std::string_view s,std::string_view prefix);
  size_t findGroupEnd(std::string_view group,size_t pos);  
  fieldKind judgeGroup(std::string_view group);
  std::optional<std::string> processFldResult(std::string_view group);
  std::vector<FieldResultGroup> loadingContainer(std::string_view group);
  void processContainer(std::string_view source , 
                        std::vector<FieldResultGroup>& container);
};