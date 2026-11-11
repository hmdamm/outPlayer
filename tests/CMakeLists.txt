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

# Hook for Task 3: test_mpv_engine target when test_mpv_engine.cpp exists
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_mpv_engine.cpp")
    qt_add_executable(test_mpv_engine
        test_mpv_engine.cpp
    )
    target_link_libraries(test_mpv_engine
        PRIVATE
            outplayer_core
            Qt6::Test
    )
    add_test(NAME test_mpv_engine COMMAND test_mpv_engine)
endif()
