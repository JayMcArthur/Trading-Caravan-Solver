cmake_minimum_required(VERSION 3.22)
project(Merchant_Trading_Caravan)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_BUILD_TYPE Debug)

# set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++ -static")
# add_compile_options(-O3)
# set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -Wextra -pedantic -fsanitize=address")

add_executable(Merchant_Trading_Caravan
        main.cpp
        player/player.h
        game/Game.cpp
        game/Game.h
        consts/enums.h
        solvers/GameSolver.cpp
        solvers/GameSolver.h
        solvers/helpers.cpp
        solvers/helpers.h
        purchaseHandler/BestPurchaseHandler.cpp
        purchaseHandler/BestPurchaseHandler.h
        purchaseHandler/FenwickTree2D.cpp
        purchaseHandler/FenwickTree2D.h
)

add_executable(Merchant_Trading_Caravan_Tests
        tests.cpp
        game/Game.cpp
        game/Game.h
        consts/enums.h
        solvers/helpers.cpp
        solvers/helpers.h
        purchaseHandler/BestPurchaseHandler.cpp
        purchaseHandler/BestPurchaseHandler.h
        purchaseHandler/FenwickTree2D.cpp
        purchaseHandler/FenwickTree2D.h
)

find_package(Python3 COMPONENTS Development QUIET)
find_package(pybind11 CONFIG QUIET)

if (Python3_FOUND AND pybind11_FOUND)
    pybind11_add_module(TCSC solvers/TCSC.cpp)
    target_link_libraries(TCSC PRIVATE pybind11::module)
endif()
