cmake_minimum_required(VERSION 3.10)
project(breakout)

find_package(raylib REQUIRED)

add_executable(breakout
    src/main.cpp
    src/Ball.cpp
    src/Paddle.cpp
    src/Brick.cpp
    src/Game.cpp
    src/PowerUp.cpp
)

target_include_directories(breakout PRIVATE include)

target_link_libraries(breakout raylib)
