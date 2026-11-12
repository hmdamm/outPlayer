# ==============================================================================
# outplayer_core library
# ==============================================================================

qt_add_library(outplayer_core STATIC
    core/CommandId.h
    core/CommandRegistry.h
    core/CommandRegistry.cpp
    core/SettingsManager.h
    core/SettingsManager.cpp
    core/BookmarkManager.h
    core/BookmarkManager.cpp
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
# outplayer_input library
# ==============================================================================

qt_add_library(outplayer_input STATIC
    input/InputManager.h
    input/InputManager.cpp
)

target_include_directories(outplayer_input
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(outplayer_input
    PUBLIC
        outplayer_core
        Qt6::Widgets
)

# ==============================================================================
# outplayer_playlist library
# ==============================================================================

qt_add_library(outplayer_playlist STATIC
    playlist/PlaylistItem.h
    playlist/PlaylistModel.h
    playlist/PlaylistModel.cpp
)

target_include_directories(outplayer_playlist
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(outplayer_playlist
    PUBLIC
        Qt6::Core
)

# ==============================================================================
# outplayer_ui library
# ==============================================================================

qt_add_library(outplayer_ui STATIC
    ui/ContextMenuManager.h
    ui/ContextMenuManager.cpp
    ui/ControlBar.h
    ui/ControlBar.cpp
    ui/CustomTitleBar.h
    ui/CustomTitleBar.cpp
    ui/MediaOpenController.h
    ui/MediaOpenController.cpp
    ui/OpenUrlDialog.h
    ui/OpenUrlDialog.cpp
    ui/OsdOverlay.h
    ui/OsdOverlay.cpp
    ui/PlaylistDock.h
    ui/PlaylistDock.cpp
    ui/TimeFormatter.h
    ui/TimelineSlider.h
    ui/TimelineSlider.cpp
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
        outplayer_input
        outplayer_playlist
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
        outplayer_input
        outplayer_playlist
        Qt6::Widgets
        Qt6::OpenGLWidgets
)
