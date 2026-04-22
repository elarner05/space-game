cmake_minimum_required(VERSION 3.11)
project(space-game VERSION 1.0)

include(FetchContent)

FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG        5.5
    GIT_SHALLOW    TRUE
)

# removes examples and games from raylib build
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(raylib)

file(GLOB_RECURSE SOURCES game/src/*.cpp)
file(GLOB_RECURSE HEADERS game/include/*.h)

add_executable(space-game ${SOURCES} ${HEADERS})

target_compile_features(space-game PRIVATE cxx_std_17)
set_target_properties(space-game PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
set_target_properties(space-game PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:space-game>"
)

add_subdirectory(libs/game-core)

target_include_directories(space-game PRIVATE ${raylib_SOURCE_DIR}/src)
target_include_directories(space-game PRIVATE libs/game-core/include)
target_link_libraries(space-game PRIVATE game-core raylib)

add_custom_command(TARGET space-game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/data
    $<TARGET_FILE_DIR:space-game>/data
)