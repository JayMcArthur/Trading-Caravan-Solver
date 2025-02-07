cmake_minimum_required(VERSION 3.22)
project(Merchant_Trading_Caravan)

set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_BUILD_TYPE Debug)

# set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++ -static")
# add_compile_options(-O3)
# set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -Wextra -pedantic -fsanitize=address")

find_package(Python3 COMPONENTS Development REQUIRED)

include_directories(${Python3_INCLUDE_DIRS})

add_executable(Merchant_Trading_Caravan
        main.cpp
        player/player.h
        game/Game.cpp
        game/Game.h
        consts/enums.h
        solvers/GameSolver.cpp
        solvers/GameSolver.h
        solvers/TCSC.cpp
        solvers/helpers.cpp
        solvers/helpers.h
        purchaseHandler/BestPurchaseHandler.cpp
        purchaseHandler/BestPurchaseHandler.h
        purchaseHandler/FenwickTree2D.cpp
        purchaseHandler/FenwickTree2D.h
)

target_include_directories(Merchant_Trading_Caravan PRIVATE ${Python3_INCLUDE_DIRS})
target_link_libraries(Merchant_Trading_Caravan PRIVATE ${Python3_LIBRARIES})
