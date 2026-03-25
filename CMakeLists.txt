cmake_minimum_required(VERSION 3.10)
project(space-game VERSION 1.0)

file(GLOB_RECURSE SOURCES game/src/*.cpp)
file(GLOB_RECURSE HEADERS game/include/*.h)

add_executable(space-game ${SOURCES} ${HEADERS})
add_subdirectory(libs/game-core)
add_subdirectory(external/raylib)

target_include_directories(space-game PRIVATE external/raylib/src)
target_link_libraries(space-game PRIVATE game-core raylib)

add_custom_command(TARGET space-game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/resources
    $<TARGET_FILE_DIR:space-game>/resources
)