cmake_minimum_required(VERSION 3.12)

option(ENABLE_TRACY "Enable Tracy Profiler" OFF)

include(FetchContent)

if(ENABLE_TRACY)
    FetchContent_Declare(
        tracy
        GIT_REPOSITORY https://github.com/wolfpld/tracy.git
        GIT_TAG master  # or a pinned stable commit
    )

    FetchContent_MakeAvailable(tracy)
endif()

file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
file(GLOB_RECURSE HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)

add_library(game-core ${SOURCES} ${HEADERS})
target_compile_features(game-core PRIVATE cxx_std_17)
set_target_properties(game-core PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)

target_include_directories(game-core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_include_directories(game-core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/vendor)

target_link_libraries(game-core PUBLIC raylib)
if(ENABLE_TRACY) 
    target_link_libraries(game-core PUBLIC TracyClient)
    target_compile_definitions(game-core PUBLIC TRACY_ENABLE)

    target_compile_definitions(TracyClient PUBLIC TRACY_ENABLE)
endif()