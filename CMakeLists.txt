cmake_minimum_required(VERSION 3.16)
project(fmu_rt_runner C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(CTest)
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBYAML REQUIRED IMPORTED_TARGET yaml-0.1)

option(MICROHIL_BUILD_RUNNER "Build the FMILibrary-based runner" ON)
option(MICROHIL_FETCH_FMILIB "Fetch the pinned FMILibrary release for the runner" OFF)
option(MICROHIL_WITH_ROS2_CONTROL "Build the ROS 2 control node used by the runner" OFF)
option(MICROHIL_BUILD_GUI "Build the Qt Quick/QML interface" OFF)

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
    src/daq_actuation.c
    src/daq_output_bridge.c
    src/daq_coordinator.c
    src/daq_protocol.c
    src/daq_schema.c
    src/daq_state_control.c
    src/daq_link_state.c
    src/daq_serial_service.c
    src/daq_tty.c
    src/daq_xrce_udp_bridge.c
    src/input_state.c
    src/log_converter.c
    src/log_format.c
    src/run_logging.c
    src/run_result.c
    src/release_schedule.c
    src/sample_queue.c
    src/sha256.c
    src/plotter.c
    src/profile_config.c
)
target_include_directories(microhil_host_components PUBLIC include)
target_link_libraries(microhil_host_components PUBLIC
    microhil_project_warnings
    Threads::Threads
    PkgConfig::LIBYAML
)

if(MICROHIL_WITH_ROS2_CONTROL)
    find_package(rcl REQUIRED)
    find_package(microhil_interfaces REQUIRED)

    add_library(microhil_ros_control STATIC src/daqc_ros_control.c)
    target_include_directories(microhil_ros_control PUBLIC include)
    target_link_libraries(microhil_ros_control PUBLIC
        microhil_project_warnings
        microhil_host_components
        rcl::rcl
        microhil_interfaces::microhil_interfaces__rosidl_generator_c
        microhil_interfaces::microhil_interfaces__rosidl_typesupport_c
        Threads::Threads
    )
endif()

if(MICROHIL_BUILD_RUNNER OR MICROHIL_BUILD_GUI)
    include(cmake/fmilib.cmake)
    microhil_resolve_fmilib(MICROHIL_FMILIB_TARGET)

    add_library(microhil_fmu_wrapper STATIC src/fmu_model.c)
    target_include_directories(microhil_fmu_wrapper PUBLIC include)
    target_link_libraries(microhil_fmu_wrapper PUBLIC
        microhil_project_warnings
        ${MICROHIL_FMILIB_TARGET}
        m
    )

    add_library(microhil_rt_simulation STATIC src/rt_simulation.c)
    target_include_directories(microhil_rt_simulation PUBLIC include)
    target_link_libraries(microhil_rt_simulation PUBLIC
        microhil_project_warnings
        microhil_host_components
        microhil_fmu_wrapper
        Threads::Threads
        m
    )

    add_library(microhil_execution_session STATIC src/execution_session.c)
    target_include_directories(microhil_execution_session PUBLIC include)
    target_link_libraries(microhil_execution_session PUBLIC microhil_project_warnings microhil_host_components microhil_fmu_wrapper microhil_rt_simulation)

    if(MICROHIL_BUILD_GUI)
        find_package(Qt6 6.2 REQUIRED COMPONENTS Quick QuickControls2)
        qt_add_executable(microhil_gui gui/main.cpp gui/gui_controller.cpp)
        set_target_properties(microhil_gui PROPERTIES AUTOMOC ON)
        qt_add_resources(microhil_gui "microhil_gui_resources" PREFIX "/" FILES gui/Main.qml gui/OutputGraphWindow.qml)
        target_link_libraries(microhil_gui PRIVATE Qt6::Quick Qt6::QuickControls2 microhil_execution_session)
    endif()

    if(MICROHIL_BUILD_RUNNER)

    add_executable(fmu_rt_runner
        src/main.c
    )
    target_include_directories(fmu_rt_runner PRIVATE include)
    target_link_libraries(fmu_rt_runner PRIVATE
        microhil_project_warnings
        microhil_host_components
        microhil_fmu_wrapper
        microhil_rt_simulation
        microhil_execution_session
        Threads::Threads
        dl
        m
    )
    if(MICROHIL_WITH_ROS2_CONTROL)
        target_compile_definitions(fmu_rt_runner PRIVATE MICROHIL_WITH_ROS2_CONTROL=1)
        target_link_libraries(fmu_rt_runner PRIVATE microhil_ros_control)
        # File capabilities suppress LD_LIBRARY_PATH; DT_RPATH also resolves transitive ROS dependencies.
        set_property(TARGET fmu_rt_runner APPEND PROPERTY BUILD_RPATH "/opt/ros/humble/lib")
        target_link_options(fmu_rt_runner PRIVATE "-Wl,--disable-new-dtags")
    endif()
    endif()
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
