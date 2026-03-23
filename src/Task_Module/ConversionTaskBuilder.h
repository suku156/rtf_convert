// =====================================================
// Module  : ConversionTaskBuilder
// Author  : suku156
// Purpose : 負責將資料建立成可執行的結構
// Layer   : Task
//
// Depend  :
//   CliRequestResolver
//
// Used by :
//   ConversionExecutor
//
// Notes :
//   負責將 Cli .Gui 拿到的資料建立成乾淨可執行的結構
//   不觸及執行的判斷的動作將在這層完成(沒指定值的某些預設動作)
// =====================================================
#pragma once
#include "ConversionTask.h"
#include "NormalizedConversionRequest.h"
#include <optional>
#include <cstddef>

namespace taskBuilder{
   class ConversionTaskBuilder{
   public:
     BuildResult build(const NormalizedConversionRequest& request);
   private:
     size_t normalizeThreadCount(std::optional<size_t> requested);
   };
}