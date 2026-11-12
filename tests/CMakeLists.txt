find_package(Qt6 6.5 REQUIRED COMPONENTS Test)

# ==============================================================================
# Phase 0 Verification Dummy Test
# ==============================================================================
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/dummy_test.cpp"
    "#include <QTest>\n\n"
    "int main(int argc, char *argv[]) {\n"
    "    return 0;\n"
    "}\n"
)

qt_add_executable(test_dummy
    "${CMAKE_CURRENT_BINARY_DIR}/dummy_test.cpp"
)

target_link_libraries(test_dummy
    PRIVATE
        outplayer_core
        Qt6::Test
)

add_test(NAME test_dummy COMMAND test_dummy)

# ==============================================================================
# Task 1: test_command_registry target
# ==============================================================================
qt_add_executable(test_command_registry
    test_command_registry.cpp
)

target_link_libraries(test_command_registry
    PRIVATE
        outplayer_core
        Qt6::Test
)

add_test(NAME test_command_registry COMMAND test_command_registry)

# ==============================================================================
# Task 2: test_settings_manager target
# ==============================================================================
qt_add_executable(test_settings_manager
    test_settings_manager.cpp
)

target_link_libraries(test_settings_manager
    PRIVATE
        outplayer_core
        Qt6::Test
)

add_test(NAME test_settings_manager COMMAND test_settings_manager)

# ==============================================================================
# Task 3: test_mpv_engine target
# ==============================================================================
qt_add_executable(test_mpv_engine
    test_mpv_engine.cpp
)

target_link_libraries(test_mpv_engine
    PRIVATE
        outplayer_core
        Qt6::Test
)

add_test(NAME test_mpv_engine COMMAND test_mpv_engine)

# ==============================================================================
# Phase 3C Task 1: test_mpv_engine_av target
# ==============================================================================
qt_add_executable(test_mpv_engine_av
    test_mpv_engine_av.cpp
)

target_link_libraries(test_mpv_engine_av
    PRIVATE
        outplayer_core
        Qt6::Test
)

add_test(NAME test_mpv_engine_av COMMAND test_mpv_engine_av)

# ==============================================================================
# Task 4: test_video_widget target
# ==============================================================================
qt_add_executable(test_video_widget
    test_video_widget.cpp
)

target_link_libraries(test_video_widget
    PRIVATE
        outplayer_ui
        Qt6::Test
)

add_test(NAME test_video_widget COMMAND test_video_widget)

# ==============================================================================
# Task 3: test_input_manager target
# ==============================================================================
qt_add_executable(test_input_manager
    test_input_manager.cpp
)

target_link_libraries(test_input_manager
    PRIVATE
        outplayer_input
        outplayer_core
        Qt6::Test
        Qt6::Widgets
)

add_test(NAME test_input_manager COMMAND test_input_manager)

# ==============================================================================
# Task 1: test_timeline_slider target
# ==============================================================================
qt_add_executable(test_timeline_slider
    test_timeline_slider.cpp
)

target_link_libraries(test_timeline_slider
    PRIVATE
        outplayer_ui
        outplayer_core
        Qt6::Test
        Qt6::Widgets
)

add_test(NAME test_timeline_slider COMMAND test_timeline_slider)

# ==============================================================================
# Task 2: test_control_bar target
# ==============================================================================
qt_add_executable(test_control_bar
    test_control_bar.cpp
)

target_link_libraries(test_control_bar
    PRIVATE
        outplayer_ui
        outplayer_core
        Qt6::Test
        Qt6::Widgets
)

add_test(NAME test_control_bar COMMAND test_control_bar)

# ==============================================================================
# Task 3: test_osd_overlay target
# ==============================================================================
qt_add_executable(test_osd_overlay
    test_osd_overlay.cpp
)

target_link_libraries(test_osd_overlay
    PRIVATE
        outplayer_ui
        outplayer_core
        Qt6::Test
        Qt6::Widgets
)

add_test(NAME test_osd_overlay COMMAND test_osd_overlay)

# ==============================================================================
# Task 4: test_main_window_ui target
# ==============================================================================
qt_add_executable(test_main_window_ui
    test_main_window_ui.cpp
)

target_link_libraries(test_main_window_ui
    PRIVATE
        outplayer_ui
        outplayer_input
        outplayer_core
        Qt6::Test
        Qt6::Widgets
)

add_test(NAME test_main_window_ui COMMAND test_main_window_ui)

# ==============================================================================
# Task 1 (Phase 3B): test_context_menu target
# ==============================================================================
qt_add_executable(test_context_menu
    test_context_menu.cpp
)

target_link_libraries(test_context_menu
    PRIVATE
        outplayer_ui
        outplayer_core
        Qt6::Test
        Qt6::Widgets
)

add_test(NAME test_context_menu COMMAND test_context_menu)

# ==============================================================================
# Task 2 (Phase 3B): test_media_open_dialogs target
# ==============================================================================
qt_add_executable(test_media_open_dialogs
    test_media_open_dialogs.cpp
)

target_link_libraries(test_media_open_dialogs
    PRIVATE
        outplayer_ui
        outplayer_core
        Qt6::Test
        Qt6::Widgets
)

add_test(NAME test_media_open_dialogs COMMAND test_media_open_dialogs)

# ==============================================================================
# Task 3 (Phase 3B): test_playlist_model target
# ==============================================================================
qt_add_executable(test_playlist_model
    test_playlist_model.cpp
)

target_link_libraries(test_playlist_model
    PRIVATE
        outplayer_playlist
        Qt6::Test
)

add_test(NAME test_playlist_model COMMAND test_playlist_model)

# ==============================================================================
# Task 4 (Phase 3B): test_playlist_dock target
# ==============================================================================
qt_add_executable(test_playlist_dock
    test_playlist_dock.cpp
)

target_link_libraries(test_playlist_dock
    PRIVATE
        outplayer_ui
        outplayer_playlist
        Qt6::Test
        Qt6::Widgets
)

add_test(NAME test_playlist_dock COMMAND test_playlist_dock)

# ==============================================================================
# Task 5 (Phase 3B): test_main_window_navigation target
# ==============================================================================
qt_add_executable(test_main_window_navigation
    test_main_window_navigation.cpp
)

target_link_libraries(test_main_window_navigation
    PRIVATE
        outplayer_ui
        outplayer_playlist
        outplayer_input
        outplayer_core
        Qt6::Test
        Qt6::Widgets
)

add_test(NAME test_main_window_navigation COMMAND test_main_window_navigation)

# ==============================================================================
# Phase 3C Task 2: test_bookmark_manager target
# ==============================================================================
qt_add_executable(test_bookmark_manager
    test_bookmark_manager.cpp
)

target_link_libraries(test_bookmark_manager
    PRIVATE
        outplayer_core
        Qt6::Test
)

add_test(NAME test_bookmark_manager COMMAND test_bookmark_manager)

# ==============================================================================
# Phase 3C Task 3: test_screen_capture target
# ==============================================================================
qt_add_executable(test_screen_capture
    test_screen_capture.cpp
)

target_link_libraries(test_screen_capture
    PRIVATE
        outplayer_ui
        outplayer_core
        Qt6::Test
        Qt6::Widgets
)

add_test(NAME test_screen_capture COMMAND test_screen_capture)

# ==============================================================================
# Phase 3C Task 4: test_control_panel target
# ==============================================================================
qt_add_executable(test_control_panel
    test_control_panel.cpp
)

target_link_libraries(test_control_panel
    PRIVATE
        outplayer_ui
        outplayer_core
        Qt6::Test
        Qt6::Widgets
)

add_test(NAME test_control_panel COMMAND test_control_panel)

# ==============================================================================
# Phase 3C Task 5: test_main_window_av target
# ==============================================================================
qt_add_executable(test_main_window_av
    test_main_window_av.cpp
)

target_link_libraries(test_main_window_av
    PRIVATE
        outplayer_ui
        outplayer_playlist
        outplayer_input
        outplayer_core
        Qt6::Test
        Qt6::Widgets
)

add_test(NAME test_main_window_av COMMAND test_main_window_av)

# ==============================================================================
# Phase 3D Task 1: test_theme_manager target
# ==============================================================================
qt_add_executable(test_theme_manager
    test_theme_manager.cpp
)

target_link_libraries(test_theme_manager
    PRIVATE
        outplayer_ui
        Qt6::Test
        Qt6::Widgets
)

add_test(NAME test_theme_manager COMMAND test_theme_manager)

# ==============================================================================
# Phase 3D Task 2: test_preferences_dialog target
# ==============================================================================
qt_add_executable(test_preferences_dialog
    test_preferences_dialog.cpp
)

target_link_libraries(test_preferences_dialog
    PRIVATE
        outplayer_ui
        outplayer_core
        outplayer_input
        Qt6::Test
        Qt6::Widgets
)

add_test(NAME test_preferences_dialog COMMAND test_preferences_dialog)

# ==============================================================================
# Phase 3D Task 3: test_device_stream_manager target
# ==============================================================================
qt_add_executable(test_device_stream_manager
    test_device_stream_manager.cpp
)

target_link_libraries(test_device_stream_manager
    PRIVATE
        outplayer_core
        Qt6::Test
)

add_test(NAME test_device_stream_manager COMMAND test_device_stream_manager)

# ==============================================================================
# Phase 3D Task 4: test_desktop_integration target
# ==============================================================================
qt_add_executable(test_desktop_integration
    test_desktop_integration.cpp
)

target_link_libraries(test_desktop_integration
    PRIVATE
        outplayer_ui
        outplayer_playlist
        outplayer_core
        Qt6::Test
        Qt6::Widgets
        Qt6::DBus
)

add_test(NAME test_desktop_integration COMMAND test_desktop_integration)

# ==============================================================================
# Phase 3D Task 5: test_main_window_integration target
# ==============================================================================
qt_add_executable(test_main_window_integration
    test_main_window_integration.cpp
)

target_link_libraries(test_main_window_integration
    PRIVATE
        outplayer_ui
        outplayer_playlist
        outplayer_input
        outplayer_core
        Qt6::Test
        Qt6::Widgets
        Qt6::DBus
)

add_test(NAME test_main_window_integration COMMAND test_main_window_integration)

# ==============================================================================
# Phase 4 Task 1: test_hotkey_parity target
# ==============================================================================
qt_add_executable(test_hotkey_parity
    test_hotkey_parity.cpp
)

target_compile_definitions(test_hotkey_parity
    PRIVATE
        PROJECT_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

target_link_libraries(test_hotkey_parity
    PRIVATE
        outplayer_input
        outplayer_core
        Qt6::Test
        Qt6::Widgets
)

add_test(NAME test_hotkey_parity COMMAND test_hotkey_parity)











