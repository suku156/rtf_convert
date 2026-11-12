# 1.測試檔案：basic_text.rtf
- 製作工具：LibreOffice Writer
- 測試範圍：基本段落、粗體、斜體、中英文混合文字
- 測試格式：TXT、Markdown、HTML
- 測試結果：三種格式皆能正確還原文字內容
- 限制：不支援字體資訊的保留與呈現

# 2.測試檔案：unicode_text.rtf
- 製作工具：LibreOffice Writer
- 測試範圍：繁體中文、日文、韓文、特殊符號、數學符號及中英文混合 Unicode 文字
- 測試格式：TXT、Markdown、HTML
- 測試結果：三種格式皆能正確處理並保留測試文字與特殊符號
- 限制：不支援字體資訊的保留與呈現

# 3.測試檔案：simple_table.rtf
- 製作工具：LibreOffice Writer
- 測試範圍：基本表格解析、列與欄結構、儲存格文字、中英文內容及表格前後段落
- 測試格式：TXT、Markdown、HTML
- 測試結果：三種格式皆能完整保留表格內容，並依據各自的表格呈現方式正確輸出
- 限制：不涉及合併儲存格、巢狀表格等進階表格結構

# 4.測試檔案：merged_cells.rtf
- 製作工具：LibreOffice Writer
- 測試範圍：水平合併儲存格、垂直合併儲存格、表格結構及表格前後中英文段落
- 測試格式：TXT、Markdown、HTML
- 測試結果：
  - TXT：水平合併儲存格解析正常；目前不支援垂直合併儲存格。表格前後中英文段落皆能正確保留。
  - Markdown：原生格式不支援合併儲存格，符合預期。表格前後中英文段落皆能正確保留。
  - HTML：水平與垂直合併儲存格皆能正確呈現，表格前後中英文段落皆能正確保留。
- 限制：TXT 格式目前不支援垂直合併儲存格；Markdown 格式受限於原生表格結構，無法呈現合併儲存格。

# 5.測試檔案：nested_table.rtf
- 製作工具：LibreOffice Writer
- 測試範圍：巢狀表格解析、外層與內層表格結構、儲存格內表格嵌入及表格前後中英文段落
- 測試格式：TXT、Markdown、HTML
- 測試結果：
  - TXT：表格前後文字及外層表格皆能正常保留；外層儲存格內的巢狀表格未能呈現。
  - Markdown：表格前後文字及外層表格皆能正常輸出；巢狀表格僅保留文字內容，無法呈現表格框架。
  - HTML：表格前後文字、外層表格及內部巢狀表格皆能正確呈現。
- 限制：html 目前單一儲存格內支援文字與巢狀表格的組合（文字 + 巢狀表格，或巢狀表格 + 文字）；若同一儲存格包含文字 + 巢狀表格 + 文字的連續結構，後續文字可能無法保留。詳細限制請參閱使用者手冊。

# 6.測試檔案：image_extraction.rtf
- 製作工具：LibreOffice Writer / LibreOffice Draw
- 測試範圍：JPG、PNG、WMF 圖片擷取、圖片格式輸出及圖片前後中英文段落
- 測試格式：TXT、Markdown、HTML
- 測試結果：
  - TXT、Markdown、HTML：皆能正確識別圖片區塊並擷取圖片。
  - JPG、PNG、WMF：皆能正確產生對應格式的圖片檔案。
  - WMF：轉換後的圖片可透過 LibreOffice Draw 正常開啟。
- 相容性觀察：轉換後的 WMF 圖檔無法透過 Windows 小畫家開啟，但可由 LibreOffice Draw 正常讀取。
- 圖片來源：自製測試圖片

# 7.測試檔案：paragraph_formatting.rtf
- 製作工具：LibreOffice Writer
- 測試範圍：一般段落、多行文字、空白段落、段落縮排及 Tab 分隔文字
- 測試格式：TXT、Markdown、HTML
- 測試結果：
  - TXT：一般段落與多行文字皆能正常保留；空白段落未能保留；段落縮排不支援；Tab 分隔文字能正確保留。
  - Markdown：一般段落與多行文字皆能正常保留；空白段落未能保留；段落縮排不支援；Tab 分隔文字未能保留。
  - HTML：一般段落與多行文字皆能正常保留；空白段落未能保留；段落縮排不支援；Tab 分隔文字能正確保留。
- 限制：目前不支援空白段落及段落縮排的保留；Markdown 輸出未能保留 Tab 分隔資訊。

# 9.測試檔案：complex_document.rtf
- 製作工具：LibreOffice Writer / LibreOffice Draw
- 測試範圍：中英文混合文字、Unicode 特殊符號、基本表格、合併儲存格、巢狀表格及 JPG、PNG、WMF 圖片擷取
- 測試格式：TXT、Markdown、HTML
- 測試結果：三種輸出格式皆能依照各自的功能支援範圍正常轉換。各項功能的轉換行為與單項測試結果一致，未觀察到因多種文件結構同時存在而產生額外的解析異常或功能衝突。
- 備註：各輸出格式的功能限制與呈現差異，請參閱對應的單項測試紀錄。