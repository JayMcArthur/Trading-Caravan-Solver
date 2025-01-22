cmake_minimum_required(VERSION 3.25)
project(Trading_Caravan_Solver)

set(CMAKE_CXX_STANDARD 17)
# set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++ -static")

add_compile_options(-O3)
add_executable(Trading_Caravan_Solver main.cpp player.h Game.cpp Game.h enums.h GameSolver.cpp GameSolver.h)

