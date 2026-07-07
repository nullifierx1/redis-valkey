add_library(${PROJECT_NAME}_lib STATIC 
    handlers/command_handler.cpp
    handlers/parser.cpp
    handlers/printer.cpp

    database/db_list_commands.cpp
    database/db_string_commands.cpp
    database/db_common_commands.cpp
    database/db_set_commands.cpp
    database/db_geo_commands.cpp
    
    database/db_helper_methods.cpp
    database/geo_parser.cpp
)

target_include_directories(${PROJECT_NAME}_lib
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)