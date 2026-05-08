cmake_minimum_required(VERSION 3.10)
project(breakout)

# 查找raylib
find_package(raylib REQUIRED)

# 使用pkg-config查找ENet
find_package(PkgConfig REQUIRED)
pkg_check_modules(ENET REQUIRED libenet)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=thread -fno-omit-frame-pointer")
set(CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fsanitize=thread")

# 如果需要线程库（某些系统需要显式链接pthread）
find_package(Threads REQUIRED)

add_executable(breakout
    src/main.cpp
    src/Ball.cpp
    src/Paddle.cpp
    src/Brick.cpp
    src/Game.cpp
    src/PowerUp.cpp
    src/NetworkManager.cpp
    src/InterpolationManager.cpp
)

# 包含目录
target_include_directories(breakout PRIVATE 
    include
    ${ENET_INCLUDE_DIRS}
)

# 链接库
target_link_libraries(breakout 
    raylib
    ${ENET_LIBRARIES}
)

# 编译选项
target_compile_options(breakout PRIVATE 
    -Wall -Wextra
    ${ENET_CFLAGS_OTHER}
)

