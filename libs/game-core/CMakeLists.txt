file(GLOB_RECURSE SOURCES src/*.cpp)
file(GLOB_RECURSE HEADERS include/*.h)

add_library(game-core ${SOURCES} ${HEADERS})
target_include_directories(game-core PUBLIC include)
target_link_libraries(game-core PUBLIC raylib)