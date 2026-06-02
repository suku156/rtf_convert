add_library(rtfcore
    ErrorSystem_Module/ErrorSystem.cpp
    LogSystem_Module/LogSystem.cpp
    FormatDetection_Module/RtfFormatDetection.cpp
    ErrorDetector_Module/ErrorDetector.cpp
    ErrorSystem_Module/ErrorHandle.cpp
    EncodingProcess_Module/Decode.cpp
    EncodingProcess_Module/GroupProcessor.cpp
    Universal_Module/Console.cpp
    TextProcessor_Module/textProcessor.cpp
    Universal_Module/OutputDirGuard.cpp
    PictProcessor_Module/pictProcessor.cpp
    SemanticStructure_Module/Document.cpp
    SemanticStructure_Module/Renderer.cpp
    MainProcess_Module/RTFProcessor.cpp
    Executor_Module/ConversionExecutor.cpp
    thread_Moudle/MyThread.cpp
    I_O_Moudle/OutputPathResolver.cpp
    Task_Module/ConversionTaskBuilder.cpp
    SheetProcessor_Module/SheetProcessor.cpp
    RtfGroupProcessor_Module/FieldGroupProcessor.cpp
)

target_include_directories(rtfcore PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(rtfcore PUBLIC
    iconv
)
