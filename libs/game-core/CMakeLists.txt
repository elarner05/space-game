file(GLOB_RECURSE SOURCES src/*.cpp)
file(GLOB_RECURSE HEADERS include/*.h)

add_library(game-core ${SOURCES} ${HEADERS})

target_compile_features(game-core PRIVATE cxx_std_17)
set_target_properties(game-core PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)

target_include_directories(game-core PUBLIC include)
target_link_libraries(game-core PUBLIC raylib)