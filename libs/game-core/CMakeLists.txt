cmake_minimum_required(VERSION 3.12)

file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
file(GLOB_RECURSE HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)

add_library(game-core ${SOURCES} ${HEADERS})
target_compile_features(game-core PRIVATE cxx_std_17)
set_target_properties(game-core PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)

target_include_directories(game-core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_include_directories(game-core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/vendor)

target_link_libraries(game-core PUBLIC raylib)