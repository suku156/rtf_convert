cmake_minimum_required(VERSION 3.16)

project(rtfconvert LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
qt_standard_project_setup()

add_library(rtfcore
    src/ErrorSystem_Module/ErrorSystem.cpp
    src/LogSystem_Module/LogSystem.cpp
    src/FormatDetection_Module/RtfFormatDetection.cpp
    src/ErrorDetector_Module/ErrorDetector.cpp
    src/ErrorSystem_Module/ErrorHandle.cpp
    src/EncodingProcess_Module/Decode.cpp
    src/EncodingProcess_Module/GroupProcessor.cpp
    src/Universal_Module/Console.cpp
    src/TextProcessor_Module/textProcessor.cpp
    src/Universal_Module/OutputDirGuard.cpp
    src/PictProcessor_Module/pictProcessor.cpp
    src/SemanticStructure_Module/Document.cpp
    src/SemanticStructure_Module/Renderer.cpp
    src/MainProcess_Module/RTFProcessor.cpp
    src/Executor_Module/ConversionExecutor.cpp
    src/thread_Moudle/MyThread.cpp
    src/I_O_Moudle/OutputPathResolver.cpp
    src/Task_Module/ConversionTaskBuilder.cpp
)

target_include_directories(rtfcore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(rtfcore PUBLIC
    iconv
)

add_executable(rtfconvert_cli
    rtfconvert.cpp
    src/Cli_Module/CliParser.cpp
    src/Cli_Module/CliRequestResolver.cpp
    
)

target_link_libraries(rtfconvert_cli PRIVATE rtfcore)

target_compile_options(rtfconvert_cli PRIVATE
    -g
    -fexec-charset=UTF-8
)

target_link_options(rtfconvert_cli PRIVATE
    -municode
    -mconsole
)

qt_add_executable(rtfconvert_gui
    Qt_gui_Module/main.cpp
    Qt_gui_Module/mainwindow.cpp
    Qt_gui_Module/mainwindow.h
    Qt_gui_Module/mainwindow.ui
    Qt_gui_Module/GuiFormData.h
    Qt_gui_Module/GuiRequestTranslator.h
)

target_link_libraries(rtfconvert_gui PRIVATE
    Qt6::Widgets
    rtfcore
)
