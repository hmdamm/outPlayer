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
