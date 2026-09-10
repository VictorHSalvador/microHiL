cmake_minimum_required(VERSION 3.8)
project(microhil_interfaces)

find_package(ament_cmake REQUIRED)
find_package(rosidl_default_generators REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
    "msg/Esp32Data.msg"
    "msg/DaqcErrors.msg"
    "msg/DaqcSetup.msg"
    "msg/DaqcState.msg"
)

ament_export_dependencies(rosidl_default_runtime)
ament_package()
