add_executable(rtfconvert_cli
    ${CMAKE_SOURCE_DIR}/rtfconvert.cpp
    CliParser.cpp
    CliRequestResolver.cpp
    ConsoleObserver.cpp
)

target_link_libraries(rtfconvert_cli PRIVATE 
    rtfcore
)

target_compile_options(rtfconvert_cli PRIVATE
    -g
    -fexec-charset=UTF-8
)

target_link_options(rtfconvert_cli PRIVATE
    -municode
    -mconsole
)

set_target_properties(rtfconvert_cli PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)