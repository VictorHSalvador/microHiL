idf_component_register(SRCS "main.c" "daqc_protocol.c" "daqc_profile.c" "daqc_transport.c" INCLUDE_DIRS "." REQUIRES driver esp_adc esp_timer micro_ros_espidf_component)
