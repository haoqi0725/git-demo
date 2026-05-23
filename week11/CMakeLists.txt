cmake_minimum_required(VERSION 3.10)
project(breakout)

# 查找raylib
find_package(raylib REQUIRED)

# 使用pkg-config查找ENet
find_package(PkgConfig REQUIRED)
pkg_check_modules(ENET REQUIRED libenet)

# ========== 修改：使用 AddressSanitizer 替代 ThreadSanitizer ==========
# 只在 Debug 模式下启用 sanitizer
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address -fno-omit-frame-pointer -g")
    set(CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fsanitize=address")
    message(STATUS "Debug build: AddressSanitizer enabled")
else()
    # 如果没有指定构建类型，默认为 Release
    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "Release")
    endif()
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
endif()

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
    Threads::Threads
)

# 编译选项
target_compile_options(breakout PRIVATE 
    -Wall -Wextra
    ${ENET_CFLAGS_OTHER}
)