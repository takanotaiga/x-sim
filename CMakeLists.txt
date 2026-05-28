cmake_minimum_required(VERSION 3.16)

project(x-sim LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(x-sim
    src/main.cpp
    src/platform/ClockLinux.cpp
    src/platform/RealtimeLinux.cpp
    src/scheduler/CyclicScheduler.cpp
    src/scheduler/TaskWorker.cpp
    src/tasks/TaskA.cpp
    src/tasks/TaskB.cpp
    src/tasks/TaskC.cpp
    src/tasks/TaskD.cpp
    src/tasks/TaskRegistry.cpp
)

target_include_directories(x-sim PRIVATE
    include
)

target_compile_options(x-sim PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

target_link_libraries(x-sim PRIVATE
    pthread
)
