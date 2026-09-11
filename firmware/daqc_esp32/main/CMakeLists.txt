idf_component_register(SRCS "main.c" "daqc_control.c" "daqc_protocol.c" "daqc_profile.c" "daqc_ros.c" "daqc_transport.c" INCLUDE_DIRS "." REQUIRES driver esp_adc esp_timer micro_ros_espidf_component)

target_include_directories(${COMPONENT_LIB} PRIVATE
    "../components/micro_ros_espidf_component/micro_ros_src/install/include/rcl"
    "../components/micro_ros_espidf_component/micro_ros_src/install/include/rclc"
    "../components/micro_ros_espidf_component/micro_ros_src/install/include/rcl_action")
