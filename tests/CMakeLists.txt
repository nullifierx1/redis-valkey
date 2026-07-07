include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0  
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(tests 
    common_commands_ut.cpp
    geo_commands_ut.cpp
    geo_parser_ut.cpp
    handlers_command_handler_ut.cpp
    handlers_parser_ut.cpp
    handlers_printer_ut.cpp
    list_commands_ut.cpp
    private_methods_ut.cpp
    set_commands_ut.cpp
    string_commands_ut.cpp
)

target_link_libraries(tests
    PRIVATE
        gtest_main  
        ${PROJECT_NAME}_lib
)

include(GoogleTest)
gtest_discover_tests(tests)
