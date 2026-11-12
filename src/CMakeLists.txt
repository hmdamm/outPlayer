# ==============================================================================
# outplayer_core library
# ==============================================================================

qt_add_library(outplayer_core STATIC
    core/CommandId.h
    core/CommandRegistry.h
    core/CommandRegistry.cpp
    core/SettingsManager.h
    core/SettingsManager.cpp
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
        Qt6::Core
        PkgConfig::MPV
)

# ==============================================================================
# outplayer_ui library
# ==============================================================================

qt_add_library(outplayer_ui STATIC
    ui/VideoWidget.h
    ui/VideoWidget.cpp
    ui/MainWindow.h
    ui/MainWindow.cpp
)

target_include_directories(outplayer_ui
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(outplayer_ui
    PUBLIC
        outplayer_core
        Qt6::Widgets
        Qt6::OpenGLWidgets
        PkgConfig::MPV
)

# ==============================================================================
# outplayer executable
# ==============================================================================

qt_add_executable(outplayer
    main.cpp
)

target_link_libraries(outplayer
    PRIVATE
        outplayer_ui
        Qt6::Widgets
        Qt6::OpenGLWidgets
)
