find_package(Qt6 REQUIRED COMPONENTS Widgets)
qt_standard_project_setup()


qt_add_executable(rtfconvert_gui
    WIN32
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    GuiFormData.h
    GuiRequestTranslator.h
    GuiRequestTranslator.cpp
    GuiObserver.h
    GuiObserver.cpp
)

target_link_libraries(rtfconvert_gui PRIVATE
    Qt6::Widgets
    rtfcore
)

set_target_properties(rtfconvert_gui PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)