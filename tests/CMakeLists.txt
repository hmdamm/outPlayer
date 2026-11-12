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


