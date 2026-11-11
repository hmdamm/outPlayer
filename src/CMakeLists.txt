# ==============================================================================
# outplayer_core library
# ==============================================================================

qt_add_library(outplayer_core STATIC
    engine/IPlaybackEngine.h
    engine/MpvEngine.h
    engine/MpvEngine.cpp
)

target_include_directories(outplayer_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(outplayer_core
    PUBLIC
        Qt6::Widgets
        Qt6::OpenGLWidgets
        PkgConfig::MPV
)

# ==============================================================================
# outplayer executable
# ==============================================================================

# Provide minimal dummy main if main.cpp is not yet implemented (Task 4)
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/dummy_main.cpp"
        "#include <QApplication>\n\n"
        "int main(int argc, char* argv[]) {\n"
        "    QApplication app(argc, argv);\n"
        "    return 0;\n"
        "}\n"
    )
    set(OUTPLAYER_APP_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/dummy_main.cpp")
else()
    set(OUTPLAYER_APP_SOURCES
        main.cpp
    )
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ui/MainWindow.cpp")
        list(APPEND OUTPLAYER_APP_SOURCES
            ui/MainWindow.h
            ui/MainWindow.cpp
        )
    endif()
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ui/VideoWidget.cpp")
        list(APPEND OUTPLAYER_APP_SOURCES
            ui/VideoWidget.h
            ui/VideoWidget.cpp
        )
    endif()
endif()

qt_add_executable(outplayer
    ${OUTPLAYER_APP_SOURCES}
)

target_link_libraries(outplayer
    PRIVATE
        outplayer_core
)
