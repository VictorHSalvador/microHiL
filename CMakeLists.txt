cmake_minimum_required(VERSION 3.16)
project(fmu_rt_runner C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

include(CTest)
find_package(Threads REQUIRED)

option(MICROHIL_BUILD_RUNNER "Build the FMILibrary-based runner" ON)
option(MICROHIL_FETCH_FMILIB "Fetch the pinned FMILibrary release for the runner" OFF)

add_library(microhil_project_warnings INTERFACE)
target_compile_options(microhil_project_warnings INTERFACE
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wconversion
)

add_library(microhil_host_components STATIC
    src/app_config.c
    src/binary_logger.c
    src/daq_acquisition.c
    src/daq_coordinator.c
    src/daq_protocol.c
    src/daq_schema.c
    src/daq_link_state.c
    src/daq_tty.c
    src/input_state.c
    src/log_converter.c
    src/log_format.c
    src/run_logging.c
    src/run_result.c
    src/release_schedule.c
    src/sample_queue.c
    src/sha256.c
    src/plotter.c
)
target_include_directories(microhil_host_components PUBLIC include)
target_link_libraries(microhil_host_components PUBLIC
    microhil_project_warnings
    Threads::Threads
)

if(MICROHIL_BUILD_RUNNER)
    include(cmake/fmilib.cmake)
    microhil_resolve_fmilib(MICROHIL_FMILIB_TARGET)

    add_library(microhil_fmu_wrapper STATIC src/fmu_model.c)
    target_include_directories(microhil_fmu_wrapper PUBLIC include)
    target_link_libraries(microhil_fmu_wrapper PUBLIC
        microhil_project_warnings
        ${MICROHIL_FMILIB_TARGET}
        m
    )

    add_executable(fmu_rt_runner
        src/main.c
        src/rt_simulation.c
    )
    target_include_directories(fmu_rt_runner PRIVATE include)
    target_link_libraries(fmu_rt_runner PRIVATE
        microhil_project_warnings
        microhil_host_components
        microhil_fmu_wrapper
        Threads::Threads
        dl
        m
    )
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
