cmake_minimum_required(VERSION 3.16)
project(MazeSolver LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MINGW)
    add_compile_options(-fexec-charset=UTF-8)
endif()

include_directories(include)

# ==========================================
# Core library (shared by all targets)
# ==========================================
add_library(maze_core STATIC
    src/maze.cpp
    src/solver.cpp
    src/evaluator.cpp
    src/terrain.cpp
    src/maze_generator.cpp
    src/dijkstra_solver.cpp
    src/astar_solver.cpp
    src/checkpoint_solver.cpp
)

# ==========================================
# Console executable (no Qt dependency)
# ==========================================
add_executable(maze_solver
    src/main.cpp
)
target_link_libraries(maze_solver maze_core)

# ==========================================
# HTTP server executable (no Qt dependency)
# ==========================================
add_executable(maze_server
    src/server.cpp
)
target_link_libraries(maze_server maze_core)
if(WIN32)
    target_link_libraries(maze_server ws2_32)
endif()

# ==========================================
# Qt6 GUI executable
# ==========================================
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_PREFIX_PATH "D:/QT/6.11.1/mingw_64")
find_package(Qt6 REQUIRED COMPONENTS Widgets)

message(STATUS "Qt6 found: ${Qt6_DIR}")
add_executable(maze_gui
    src/qt_main.cpp
    src/mainwindow.cpp
    include/mainwindow.h
)
target_link_libraries(maze_gui maze_core Qt6::Widgets)
