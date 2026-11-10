RTF Converter

A cross-platform RTF document converter developed in C++20, featuring a command-line interface (CLI) and a Qt 6 graphical user interface (GUI).

RTF Converter 是一套以 C++20 開發的文件轉換工具，提供命令列介面（CLI）與圖形使用者介面（GUI），可將 RTF 文件轉換為 TXT、Markdown 與 HTML。

本專案著重於複雜 RTF 文件的解析、編碼處理、表格結構、圖片擷取、批次處理，以及 Windows 與 Linux 的跨平台相容性。

## Features :
  . Multiple output formats — TXT、Markdown、HTML
  . Dual interfaces — CLI 與 Qt GUI
  . Text encoding handling — 處理不同來源的文字編碼及 Unicode 內容
  . Table processing — 支援表格解析、儲存格合併及巢狀表格處理
  . Image extraction — 擷取文件中的圖片，包含 WMF、EMF 與 PNG 等格式
  . Batch processing — 支援單一檔案及資料夾批次轉換
  . Multithreading — 支援多執行緒批次處理
  . Progress and error reporting — 提供進度回報、日誌與錯誤資訊
  . Cross-platform build — 已在 Windows 與 Ubuntu Linux 進行建置及功能測試

## Technology Stack :
  . C++20 -- Core application development
  . CMake -- Cross-platform build configuration
  . GCC/MinGW-w64 -- C++ compilation on Linux and Windows
  . Qt 6 Widgets -- Graphical user interface (GUI)
  . Git -- Version control
  . iconv/platform-specific APIs -- Text encoding conversion

## Architecture :
  . The project separates document processing from the user interfaces.

```text
CLI                        Qt GUI
   |                           |
   +-------------+-------------+
                 |
           Shared Core
                 |
   +-------------+------------------+
   |             |                  |
 RTF Parsing   Image Processing  Rendering
   |             |                  |
   +-------------+------------------+
                 |
        TXT / Markdown / HTML
```

 The shared core handles document processing, while CLI and GUI provide different ways to interact with the same conversion functionality.  

 共用核心負責文件處理，而 CLI 與 GUI 則提供不同的方式，讓使用者操作相同的轉換功能 

## 限制事項 | Limitations
- 目前版本尚未支援 RTF 文件中的圖形資料解析與轉換。遇到相關圖形內容時，轉換結果可能不會保留該圖形。
- The current version does not support parsing or converting graphical data in RTF documents. Such graphical content may not be preserved in the conversion output. 

## 使用者使用說明 Documentation

 [Windows 一般使用者使用說明(Windows User Guide)](docs/Windows_User_Guide.md)

 [Linux 一般使用者使用說明(Linux User Guide)](docs/Linux_User_Guide.md) 

開發者文件 Developer Guide

 [開發環境設定與編譯說明(Developer Guide)](docs/Developer_Guide.md)

預編譯版本下載 Prebuilt Releases

[前往 GitHub Releases 下載對應平台的預編譯版本](../../releases)
 . Download the precompiled version for your platform from GitHub Releases.

