cmake_minimum_required(VERSION 3.16)
project(fmu_rt_runner C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(FMILIB_ROOT "" CACHE PATH "FMI Library installation prefix")

find_path(FMILIB_INCLUDE_DIR
    NAMES fmilib.h
    HINTS ${FMILIB_ROOT}/include $ENV{FMILIB_ROOT}/include
)
find_library(FMILIB_LIBRARY
    NAMES fmilib fmilib_shared
    HINTS ${FMILIB_ROOT}/lib ${FMILIB_ROOT}/lib64 $ENV{FMILIB_ROOT}/lib $ENV{FMILIB_ROOT}/lib64
)

if(NOT FMILIB_INCLUDE_DIR OR NOT FMILIB_LIBRARY)
    message(FATAL_ERROR "FMI Library not found. Set -DFMILIB_ROOT=/path/to/fmilib/install")
endif()

find_package(Threads REQUIRED)

add_executable(fmu_rt_runner
    src/main.c
    src/app_config.c
    src/sample_queue.c
    src/fmu_model.c
    src/rt_simulation.c
    src/csv_logger.c
    src/plotter.c
)

target_include_directories(fmu_rt_runner PRIVATE include ${FMILIB_INCLUDE_DIR})
target_compile_options(fmu_rt_runner PRIVATE -Wall -Wextra -Wpedantic -Wshadow -Wconversion)
target_link_libraries(fmu_rt_runner PRIVATE ${FMILIB_LIBRARY} Threads::Threads dl m)
