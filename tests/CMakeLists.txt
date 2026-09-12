add_executable(test_sample_queue test_sample_queue.c)
target_link_libraries(test_sample_queue PRIVATE microhil_host_components)

add_executable(test_log_format test_log_format.c)
target_link_libraries(test_log_format PRIVATE microhil_host_components m)

add_executable(test_binary_logger test_binary_logger.c)
target_link_libraries(test_binary_logger PRIVATE microhil_host_components m)

add_executable(test_log_converter test_log_converter.c)
target_link_libraries(test_log_converter PRIVATE microhil_host_components m)

add_executable(test_run_logging test_run_logging.c)
target_link_libraries(test_run_logging PRIVATE microhil_host_components m)

add_executable(test_input_state test_input_state.c)
target_link_libraries(test_input_state PRIVATE microhil_host_components m)

add_executable(test_daq_protocol test_daq_protocol.c)
target_link_libraries(test_daq_protocol PRIVATE microhil_host_components m)

add_executable(test_daq_tty test_daq_tty.c)
target_link_libraries(test_daq_tty PRIVATE microhil_host_components m)

add_executable(test_daq_serial_service test_daq_serial_service.c)
target_link_libraries(test_daq_serial_service PRIVATE microhil_host_components m)

add_executable(test_daq_schema test_daq_schema.c)
target_link_libraries(test_daq_schema PRIVATE microhil_host_components m)

add_executable(test_daq_link_state test_daq_link_state.c)
target_link_libraries(test_daq_link_state PRIVATE microhil_host_components m)

add_executable(test_daq_acquisition test_daq_acquisition.c)
target_link_libraries(test_daq_acquisition PRIVATE microhil_host_components m)

add_executable(test_daq_actuation test_daq_actuation.c)
target_link_libraries(test_daq_actuation PRIVATE microhil_host_components m)

add_executable(test_daq_output_bridge test_daq_output_bridge.c)
target_link_libraries(test_daq_output_bridge PRIVATE microhil_host_components m)

add_executable(test_daq_coordinator test_daq_coordinator.c)
target_link_libraries(test_daq_coordinator PRIVATE microhil_host_components m)

add_executable(test_daq_state_control test_daq_state_control.c)
target_link_libraries(test_daq_state_control PRIVATE microhil_host_components m)

add_executable(test_daq_xrce_udp_bridge test_daq_xrce_udp_bridge.c)
target_link_libraries(test_daq_xrce_udp_bridge PRIVATE microhil_host_components m)

add_executable(test_app_config test_app_config.c)
target_link_libraries(test_app_config PRIVATE microhil_host_components m)

add_executable(test_release_schedule test_release_schedule.c)
target_link_libraries(test_release_schedule PRIVATE microhil_host_components m)

add_executable(test_profile_config test_profile_config.c)
target_link_libraries(test_profile_config PRIVATE microhil_host_components m)

if(TARGET microhil_fmu_wrapper)
    add_executable(test_fmu_model test_fmu_model.c)
    target_link_libraries(test_fmu_model PRIVATE microhil_rt_simulation microhil_fmu_wrapper microhil_host_components m)

    add_executable(test_execution_session test_execution_session.c)
    target_link_libraries(test_execution_session PRIVATE microhil_execution_session)
endif()

if(TARGET microhil_ros_control)
    add_executable(test_daqc_ros_control test_daqc_ros_control.c)
    target_link_libraries(test_daqc_ros_control PRIVATE microhil_ros_control)
endif()

add_test(NAME sample_queue_spsc_order COMMAND test_sample_queue order)
add_test(NAME sample_queue_spsc_wrap COMMAND test_sample_queue wrap)
add_test(NAME sample_queue_spsc_saturation COMMAND test_sample_queue saturation)
add_test(NAME sample_queue_spsc_concurrent COMMAND test_sample_queue concurrent)
add_test(NAME log_format_header COMMAND test_log_format header)
add_test(NAME log_format_records COMMAND test_log_format records)
add_test(NAME log_format_rejection COMMAND test_log_format rejection)
add_test(NAME binary_logger_file COMMAND test_binary_logger file)
add_test(NAME binary_logger_saturation COMMAND test_binary_logger saturation)
add_test(NAME binary_logger_wrap COMMAND test_binary_logger wrap)
add_test(NAME binary_logger_interleaving COMMAND test_binary_logger interleaving)
add_test(NAME binary_logger_sink_failures COMMAND test_binary_logger failures)
add_test(NAME log_converter_running COMMAND test_log_converter running)
add_test(NAME log_converter_schema COMMAND test_log_converter schema)
add_test(NAME log_converter_csv COMMAND test_log_converter csv)
add_test(NAME log_converter_sequences COMMAND test_log_converter sequences)
add_test(NAME log_converter_truncated COMMAND test_log_converter truncated)
add_test(NAME run_logging_enabled COMMAND test_run_logging enabled)
add_test(NAME run_logging_disabled COMMAND test_run_logging disabled)
add_test(NAME run_logging_saturation COMMAND test_run_logging saturation)
add_test(NAME run_logging_failure COMMAND test_run_logging failure)
add_test(NAME run_logging_descriptor COMMAND test_run_logging descriptor)
add_test(NAME run_logging_aggregate COMMAND test_run_logging aggregate)
add_test(NAME input_state_retention COMMAND test_input_state retention)
add_test(NAME input_state_snapshot COMMAND test_input_state snapshot)
add_test(NAME input_state_protection COMMAND test_input_state protection)
add_test(NAME daq_protocol_frames COMMAND test_daq_protocol frames)
add_test(NAME daq_protocol_fragmentation COMMAND test_daq_protocol fragmentation)
add_test(NAME daq_protocol_sequence COMMAND test_daq_protocol sequence)
add_test(NAME daq_protocol_xrce_mtu COMMAND test_daq_protocol xrce_mtu)
add_test(NAME daq_tty_validation COMMAND test_daq_tty)
add_test(NAME daq_serial_service_round_trip COMMAND test_daq_serial_service)
add_test(NAME daq_schema_validation COMMAND test_daq_schema)
add_test(NAME daq_link_state_transitions COMMAND test_daq_link_state)
add_test(NAME daq_acquisition_snapshot COMMAND test_daq_acquisition)
add_test(NAME daq_actuation_payload COMMAND test_daq_actuation)
add_test(NAME daq_output_bridge_streaming COMMAND test_daq_output_bridge)
add_test(NAME daq_coordinator_demultiplexing COMMAND test_daq_coordinator)
add_test(NAME daq_state_control_transitions COMMAND test_daq_state_control)
add_test(NAME daq_xrce_udp_bridge_loopback COMMAND test_daq_xrce_udp_bridge)
set_tests_properties(daq_xrce_udp_bridge_loopback PROPERTIES SKIP_RETURN_CODE 77)
add_test(NAME app_config_timing COMMAND test_app_config)
add_test(NAME release_schedule_overrun COMMAND test_release_schedule overrun)
add_test(NAME release_schedule_boundary COMMAND test_release_schedule boundary)
add_test(NAME release_schedule_wakeup COMMAND test_release_schedule wakeup)
add_test(NAME profile_config_yaml COMMAND test_profile_config "${CMAKE_SOURCE_DIR}/tests/fixtures/esp32_profile.yaml"
         "${CMAKE_SOURCE_DIR}/tests/fixtures/esp32_profile_legacy_acquisition.yaml"
         "${CMAKE_SOURCE_DIR}/tests/fixtures/esp32_profile_missing_adc.yaml"
         "${CMAKE_SOURCE_DIR}/tests/fixtures/esp32_profile_physical_integer.yaml"
         "${CMAKE_SOURCE_DIR}/tests/fixtures/esp32_profile_invalid_acquisition.yaml")

if(TARGET test_fmu_model)
    add_test(NAME fmu_model_inputs COMMAND test_fmu_model "${CMAKE_SOURCE_DIR}/tests/fixtures/teste001.fmu" "${CMAKE_SOURCE_DIR}/tests/fixtures/esp32_profile.yaml")
    add_test(NAME execution_session_lifecycle COMMAND test_execution_session "${CMAKE_SOURCE_DIR}/tests/fixtures/teste001.fmu" "${CMAKE_SOURCE_DIR}/tests/fixtures/esp32_profile.yaml")
endif()

if(TARGET test_daqc_ros_control)
    add_test(NAME daqc_ros_control_state COMMAND test_daqc_ros_control)
endif()
