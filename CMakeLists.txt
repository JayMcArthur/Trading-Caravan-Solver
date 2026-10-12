cmake_minimum_required(VERSION 3.22)
project(Merchant_Trading_Caravan)

find_package(SQLite3 REQUIRED)
find_package(ZLIB REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # Serialize and cap native tool processes even if a caller passes a high -j.
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CMAKE_SOURCE_DIR}/scripts/compiler_limiter.sh")
    set(CMAKE_CXX_LINKER_LAUNCHER "${CMAKE_SOURCE_DIR}/scripts/compiler_limiter.sh")
endif()
# set(CMAKE_BUILD_TYPE Debug)

# set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++ -static")
# add_compile_options(-O3)
# set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -Wextra -pedantic -fsanitize=address")

add_executable(Merchant_Trading_Caravan
        main.cpp
        benchmarks/NormaliaBenchmark.cpp
        benchmarks/NormaliaBenchmark.h
        benchmarks/RecordRoutes.cpp
        benchmarks/RecordRoutes.h
        player/player.h
        game/Game.cpp
        game/Game.h
        consts/enums.h
        solvers/GameSolver.cpp
        solvers/GameSolver.h
        solvers/DeferredGoldFunction.cpp
        solvers/DeferredGoldFunction.h
        solvers/DeferredTraceRecipe.cpp
        solvers/DeferredTraceRecipe.h
        solvers/DeferredPurchaseModel.cpp
        solvers/DeferredPurchaseModel.h
        solvers/DeferredSoldModel.cpp
        solvers/DeferredSoldModel.h
        solvers/DeferredSearchState.cpp
        solvers/DeferredSearchState.h
        solvers/EarlyActionSkeleton.cpp
        solvers/EarlyActionSkeleton.h
        solvers/EncounterTiming.cpp
        solvers/EncounterTiming.h
        solvers/ContinuationMatrixSelector.cpp
        solvers/ContinuationMatrixSelector.h
        solvers/FrontierReducer.cpp
        solvers/FrontierReducer.h
        solvers/FrontierAccumulator.cpp
        solvers/AdmissibleUpperBound.cpp
        solvers/FrontierAccumulator.h
        solvers/RouteSurvivalAudit.cpp
        solvers/RouteSurvivalAudit.h
        solvers/SkeletonWorkQueue.cpp
        solvers/SkeletonWorkQueue.h
        solvers/ValidatedRouteReplay.cpp
        solvers/ValidatedRouteReplay.h
        solvers/RoutePolisher.cpp
        solvers/RoutePolisher.h
        solvers/SolverAction.h
        solvers/SolverStateCodec.cpp
        solvers/SolverStateCodec.h
        solvers/PersistentSearchGraph.cpp
        solvers/PersistentSearchGraph.h
        solvers/helpers.cpp
        solvers/helpers.h
        purchaseHandler/BestPurchaseHandler.cpp
        purchaseHandler/BestPurchaseHandler.h
        purchaseHandler/FenwickTree2D.cpp
        purchaseHandler/FenwickTree2D.h
        runtime/ProcessSafety.cpp
        runtime/ProcessSafety.h
)

add_executable(Merchant_Trading_Caravan_Tests
        tests.cpp
        tests/DeferredPurchaseOracle.cpp
        tests/DeferredPurchaseOracle.h
        tests/DeferredPurchaseModelTests.cpp
        tests/DeferredPurchaseModelTests.h
        benchmarks/NormaliaBenchmark.cpp
        benchmarks/NormaliaBenchmark.h
        benchmarks/RecordRoutes.cpp
        benchmarks/RecordRoutes.h
        game/Game.cpp
        game/Game.h
        consts/enums.h
        solvers/GameSolver.cpp
        solvers/GameSolver.h
        solvers/DeferredGoldFunction.cpp
        solvers/DeferredGoldFunction.h
        solvers/DeferredTraceRecipe.cpp
        solvers/DeferredTraceRecipe.h
        solvers/DeferredPurchaseModel.cpp
        solvers/DeferredPurchaseModel.h
        solvers/DeferredSoldModel.cpp
        solvers/DeferredSoldModel.h
        solvers/DeferredSearchState.cpp
        solvers/DeferredSearchState.h
        solvers/EarlyActionSkeleton.cpp
        solvers/EarlyActionSkeleton.h
        solvers/EncounterTiming.cpp
        solvers/EncounterTiming.h
        solvers/ContinuationMatrixSelector.cpp
        solvers/ContinuationMatrixSelector.h
        solvers/FrontierReducer.cpp
        solvers/FrontierReducer.h
        solvers/FrontierAccumulator.cpp
        solvers/AdmissibleUpperBound.cpp
        solvers/FrontierAccumulator.h
        solvers/RouteSurvivalAudit.cpp
        solvers/RouteSurvivalAudit.h
        solvers/SkeletonWorkQueue.cpp
        solvers/SkeletonWorkQueue.h
        solvers/ValidatedRouteReplay.cpp
        solvers/ValidatedRouteReplay.h
        solvers/RoutePolisher.cpp
        solvers/RoutePolisher.h
        solvers/SolverAction.h
        solvers/SolverStateCodec.cpp
        solvers/SolverStateCodec.h
        solvers/PersistentSearchGraph.cpp
        solvers/PersistentSearchGraph.h
        solvers/helpers.cpp
        solvers/helpers.h
        purchaseHandler/BestPurchaseHandler.cpp
        purchaseHandler/BestPurchaseHandler.h
        purchaseHandler/FenwickTree2D.cpp
        purchaseHandler/FenwickTree2D.h
        runtime/ProcessSafety.cpp
        runtime/ProcessSafety.h
)

target_link_libraries(Merchant_Trading_Caravan PRIVATE SQLite::SQLite3 ZLIB::ZLIB)
target_link_libraries(Merchant_Trading_Caravan_Tests PRIVATE SQLite::SQLite3 ZLIB::ZLIB)

enable_testing()
add_test(
    NAME safe_native_tests
    COMMAND "${CMAKE_SOURCE_DIR}/scripts/run_tests_safe.sh"
)
set_tests_properties(safe_native_tests PROPERTIES TIMEOUT 190)
add_test(
    NAME safe_job_limits_tests
    COMMAND "${CMAKE_SOURCE_DIR}/scripts/run_tests_safe.sh" safety-limits
)
set_tests_properties(safe_job_limits_tests PROPERTIES TIMEOUT 190)
add_test(
    NAME safe_prefix_timing_tests
    COMMAND "${CMAKE_SOURCE_DIR}/scripts/run_tests_safe.sh" prefix-timing
)
set_tests_properties(safe_prefix_timing_tests PROPERTIES TIMEOUT 190)

add_custom_target(
    safe-tests
    COMMAND "${CMAKE_SOURCE_DIR}/scripts/run_tests_safe.sh"
    COMMAND "${CMAKE_SOURCE_DIR}/scripts/run_tests_safe.sh" safety-limits
    COMMAND "${CMAKE_SOURCE_DIR}/scripts/run_tests_safe.sh" prefix-timing
    DEPENDS Merchant_Trading_Caravan_Tests Merchant_Trading_Caravan
    USES_TERMINAL
)

if (MSVC)
    target_compile_options(Merchant_Trading_Caravan_Tests PRIVATE /UNDEBUG)
else()
    target_compile_options(Merchant_Trading_Caravan_Tests PRIVATE -UNDEBUG)
endif()

find_package(Python3 COMPONENTS Development QUIET)
find_package(pybind11 CONFIG QUIET)

if (Python3_FOUND AND pybind11_FOUND)
    pybind11_add_module(TCSC solvers/TCSC.cpp runtime/ProcessSafety.cpp runtime/ProcessSafety.h)
    target_link_libraries(TCSC PRIVATE pybind11::module)
endif()
