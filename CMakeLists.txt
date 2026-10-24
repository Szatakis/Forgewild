cmake_minimum_required(VERSION 3.20)

project(Forgewild LANGUAGES C CXX)

include(CTest)
include(FetchContent)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
foreach(configuration DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${configuration} "${CMAKE_CURRENT_BINARY_DIR}")
endforeach()
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(raylib)

add_executable(Forgewild main.cpp game.cpp)
target_link_libraries(Forgewild PRIVATE raylib)
target_compile_definitions(Forgewild PRIVATE
    FORGEWILD_DATA_DIR="${CMAKE_CURRENT_BINARY_DIR}"
)

if(MSVC)
    target_compile_options(Forgewild PRIVATE /W4 /permissive-)
else()
    target_compile_options(Forgewild PRIVATE -Wall -Wextra -Wpedantic)
endif()

foreach(data_directory Textures Audio)
    add_custom_command(TARGET Forgewild POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/${data_directory}"
            "$<TARGET_FILE_DIR:Forgewild>/${data_directory}"
        VERBATIM
    )
endforeach()
add_custom_command(TARGET Forgewild POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:Forgewild>/PDD"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/PDD/credits.txt"
        "$<TARGET_FILE_DIR:Forgewild>/PDD/credits.txt"
    VERBATIM
)
add_custom_command(TARGET Forgewild POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/font.TTF"
        "$<TARGET_FILE_DIR:Forgewild>/font.TTF"
    VERBATIM
)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/PDD")
foreach(save_file map_data.txt player_stats.txt)
    if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/PDD/${save_file}")
        configure_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/PDD/${save_file}"
            "${CMAKE_CURRENT_BINARY_DIR}/PDD/${save_file}"
            COPYONLY
        )
    endif()
endforeach()

if(BUILD_TESTING)
    add_executable(ForgewildTests game_tests.cpp game.cpp)
    if(MSVC)
        target_compile_options(ForgewildTests PRIVATE /W4 /permissive-)
    else()
        target_compile_options(ForgewildTests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME ForgewildGameLogic COMMAND ForgewildTests)
endif()
