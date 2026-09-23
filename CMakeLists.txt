cmake_minimum_required(VERSION 3.16)
project(chess LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
    add_compile_options(/W4 /permissive- /EHsc)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(SFML 2.6 COMPONENTS window graphics audio system REQUIRED)

add_executable(chess
    src/main.cpp
    src/chess/chess.cpp
    src/chess/fen.cpp
    src/engine/eval.cpp
    src/engine/search.cpp
    src/ui/theme.cpp
    src/ui/render.cpp
    src/ui/pieces.cpp
    src/ui/sound.cpp
    src/ui/app.cpp
    src/ui/menu.cpp
    src/io/opening_book.cpp
    src/io/persistence.cpp
)

target_include_directories(chess PRIVATE src)
target_link_libraries(chess PRIVATE sfml-window sfml-graphics sfml-audio sfml-system)