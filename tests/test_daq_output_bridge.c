#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "daq_output_bridge.h"

static void Require(bool condition, const char *message) {
    if (!condition) {
        fprintf(stderr, "%s\n", message);
        exit(EXIT_FAILURE);
    }
}

int main(void) {
    const input_channel_descriptor_t inputs[] = {
        {.input_name = "input", .value_reference = 1U, .type = NUMERIC_REAL, .initial_value = {.real_value = 0.0}, .initial_value_valid = true}
    };
    const daq_field_t acquisition_fields[] = {
        {.gpio = 32U, .function = DAQ_CHANNEL_AI, .wire_type = DAQ_WIRE_FLOAT32, .fmu_type = NUMERIC_REAL,
         .offset = 4U, .width = 4U, .scale = 1.0, .offset_value = 0.0, .fmu_index = 0U}
    };
    const daq_field_t actuation_fields[] = {
        {.gpio = 16U, .function = DAQ_CHANNEL_DO, .wire_type = DAQ_WIRE_BOOLEAN, .fmu_type = NUMERIC_BOOLEAN,
         .offset = 0U, .width = 1U, .scale = 1.0, .offset_value = 0.0, .value_reference = 7U, .fmu_index = 0U}
    };
    const OutputVariable outputs[] = {{.name = "output", .xml_index = 2U, .value_reference = 7U, .fmu_index = 0U, .type = NUMERIC_BOOLEAN}};
    const log_value_t values[] = {{.boolean_value = 1U}};
    daq_schema_t acquisition_schema;
    daq_schema_t actuation_schema;
    input_state_t input_state;
    daq_coordinator_t coordinator;
    daq_output_bridge_t bridge;
    uint8_t bytes[DAQ_PROTOCOL_DATA_PREFIX_SIZE + DAQ_PROTOCOL_MAX_DATA_PAYLOAD];
    daq_transmit_frame_t frame;

    Require(DaqSchemaBuildFixedPayload(&acquisition_schema, acquisition_fields, 1U, 28U) == DAQ_SCHEMA_OK, "could not build acquisition schema");
    Require(DaqSchemaBuildFixedPayload(&actuation_schema, actuation_fields, 1U, 21U) == DAQ_SCHEMA_OK, "could not build actuation schema");
    Require(InputStateInit(&input_state, inputs, 1U, false) == INPUT_STATE_STATUS_OK, "could not initialize input state");
    const daq_coordinator_config_t config = {.acquisition_schema = &acquisition_schema, .input_state = &input_state, .xrce_mtu = DAQ_PROTOCOL_XRCE_MTU};
    Require(DaqCoordinatorInit(&coordinator, &config) == DAQ_COORDINATOR_OK, "could not initialize coordinator");
    Require(DaqOutputBridgeInit(&bridge, &actuation_schema, &coordinator) == DAQ_OUTPUT_BRIDGE_OK, "could not initialize output bridge");
    Require(DaqOutputBridgePublish(&bridge, outputs, values, 1U) == DAQ_OUTPUT_BRIDGE_COORDINATOR,
            "actuation was accepted before STREAMING confirmation");
    Require(DaqLinkStateConfirm(&coordinator.link_state, DAQ_PROTOCOL_COMMAND_ENABLE, DAQ_PROTOCOL_COMMAND_ENABLE) == DAQ_LINK_OK,
            "could not confirm ENABLE");
    Require(DaqLinkStateConfirm(&coordinator.link_state, DAQ_PROTOCOL_COMMAND_STREAMING, DAQ_PROTOCOL_COMMAND_STREAMING) == DAQ_LINK_OK,
            "could not confirm STREAMING");
    Require(DaqOutputBridgePublish(&bridge, outputs, values, 1U) == DAQ_OUTPUT_BRIDGE_OK, "could not publish streaming output");
    Require(DaqCoordinatorTakeTransmit(&coordinator, bytes, sizeof(bytes), &frame) == DAQ_COORDINATOR_OK && frame.kind == DAQ_TRANSMIT_DATA &&
            frame.size == 26U && bytes[3] == 0U && bytes[4] == 0U && bytes[5] == 1U, "published output DATA frame is incorrect");
    Require(DaqOutputBridgePublishZero(&bridge) == DAQ_OUTPUT_BRIDGE_OK, "could not queue zero output frame");
    Require(DaqCoordinatorTakeTransmit(&coordinator, bytes, sizeof(bytes), &frame) == DAQ_COORDINATOR_OK && frame.kind == DAQ_TRANSMIT_DATA &&
            bytes[3] == 1U && bytes[4] == 0U && bytes[5] == 0U, "zero output DATA frame is incorrect");
    DaqCoordinatorDestroy(&coordinator);
    InputStateDestroy(&input_state);
    return EXIT_SUCCESS;
}
