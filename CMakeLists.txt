cmake_minimum_required(VERSION 4.3.0)

project(rtfconvert LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(rtfconvert_cli
    rtfconvert.cpp
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
    src/Cli_Module/CliParser.cpp
    src/MainProcess_Module/RTFProcessor.cpp
    src/Executor_Module/ConversionExecutor.cpp
    src/thread_Moudle/MyThread.cpp
    src/Cli_Module/CliRequestResolver.cpp
    src/I_O_Moudle/OutputPathResolver.cpp
    src/Task_Module/ConversionTaskBuilder.cpp
)

target_include_directories(rtfconvert_cli PRIVATE
    src
    src/ErrorSystem_Module
    src/LogSystem_Module
    src/FormatDetection_Module
    src/ErrorDetector_Module
    src/EncodingProcess_Module
    src/Universal_Module
    src/TextProcessor_Module
    src/PictProcessor_Module
    src/SemanticStructure_Module
    src/Cli_Module
    src/MainProcess_Module
    src/Executor_Module
    src/thread_Moudle
    src/I_O_Moudle
    src/Task_Module
)

target_compile_options(rtfconvert_cli PRIVATE
    -g
    -fexec-charset=UTF-8
)

target_link_options(rtfconvert_cli PRIVATE
    -municode
    -mconsole
)

target_link_libraries(rtfconvert_cli PRIVATE
    iconv
)