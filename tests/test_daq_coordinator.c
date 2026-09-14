#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "daq_coordinator.h"

typedef struct {
    size_t count;
    uint8_t first_bytes[4];
} xrce_capture_t;

static void Require(bool condition, const char *message) {
    if (!condition) {
        fprintf(stderr, "%s\n", message);
        exit(EXIT_FAILURE);
    }
}

static bool CaptureXrce(const uint8_t *payload, size_t payload_size, void *context) {
    xrce_capture_t *capture = context;
    if (capture->count < sizeof(capture->first_bytes)) capture->first_bytes[capture->count] = payload_size > 0U ? payload[0] : 0U;
    ++capture->count;
    return true;
}

int main(void) {
    const input_channel_descriptor_t inputs[] = {
        {.input_name = "input", .value_reference = 1U, .type = NUMERIC_REAL, .initial_value = {.real_value = 0.0}, .initial_value_valid = true}
    };
    const daq_field_t fields[] = {
        {.gpio = 34U, .function = DAQ_CHANNEL_AI, .wire_type = DAQ_WIRE_FLOAT32, .fmu_type = NUMERIC_REAL,
         .offset = 0U, .width = 4U, .scale = 1.0, .offset_value = 0.0, .fmu_index = 0U}
    };
    const uint8_t acquisition_payload[] = {0U, 0U, 0x20U, 0x40U};
    const uint8_t xrce_payload[] = {0xa5U, 0x11U};
    const uint8_t outbound_xrce_payload[] = {0x42U, 0x24U};
    const uint8_t outbound_xrce_followup[] = {0x43U, 0x25U};
    const uint8_t output_payload[] = {7U, 8U};
    uint8_t incoming[64];
    uint8_t outgoing[DAQ_PROTOCOL_DATA_PREFIX_SIZE + DAQ_PROTOCOL_MAX_DATA_PAYLOAD];
    size_t incoming_size = 0U;
    daq_schema_t schema;
    input_state_t input_state;
    input_step_t step;
    xrce_capture_t capture = {0};
    daq_coordinator_t coordinator;
    daq_transmit_frame_t frame;
    daq_coordinator_config_t config = {
        .acquisition_schema = &schema,
        .input_state = &input_state,
        .xrce_mtu = DAQ_PROTOCOL_XRCE_MTU,
        .xrce_receive = CaptureXrce,
        .xrce_context = &capture
    };

    Require(DaqSchemaBuild(&schema, fields, 1U) == DAQ_SCHEMA_OK, "could not build schema");
    Require(InputStateInit(&input_state, inputs, 1U, true) == INPUT_STATE_STATUS_OK, "could not initialize input state");
    config.xrce_mtu = 64U;
    Require(DaqCoordinatorInit(&coordinator, &config) == DAQ_COORDINATOR_INVALID_ARGUMENT, "coordinator accepted a mismatched XRCE MTU");
    config.xrce_mtu = DAQ_PROTOCOL_XRCE_MTU;
    Require(DaqCoordinatorInit(&coordinator, &config) == DAQ_COORDINATOR_OK, "could not initialize coordinator");
    Require(DaqCoordinatorReceive(&coordinator, acquisition_payload, sizeof(acquisition_payload)) == DAQ_COORDINATOR_OK &&
            coordinator.rejected_frames == 0U, "unframed bytes were not handled by the parser");
    Require(DaqProtocolEncodeConfig(incoming + incoming_size, sizeof(incoming) - incoming_size, DAQ_PROTOCOL_COMMAND_ENABLE, true,
                                    DAQ_PROTOCOL_COMMAND_ENABLE) == 5U, "could not encode ENABLE confirmation");
    incoming_size += 5U;
    Require(DaqProtocolEncodeConfig(incoming + incoming_size, sizeof(incoming) - incoming_size, DAQ_PROTOCOL_COMMAND_STREAMING, true,
                                    DAQ_PROTOCOL_COMMAND_STREAMING) == 5U, "could not encode STREAMING confirmation");
    incoming_size += 5U;
    Require(DaqProtocolEncodeData(incoming + incoming_size, sizeof(incoming) - incoming_size, 9U, acquisition_payload,
                                  sizeof(acquisition_payload)) == 9U, "could not encode acquisition DATA");
    incoming_size += 9U;
    Require(DaqProtocolEncodeXrce(incoming + incoming_size, sizeof(incoming) - incoming_size, xrce_payload, sizeof(xrce_payload)) == 7U,
            "could not encode XRCE frame");
    incoming_size += 7U;
    Require(DaqCoordinatorReceive(&coordinator, incoming, incoming_size) == DAQ_COORDINATOR_OK, "valid inbound frames were rejected");
    Require(InputStatePrepareStep(&input_state, &step) == INPUT_STATE_STATUS_OK && fabs(step.values[0].real_value - 2.5) < 1e-12,
            "inbound DATA did not reach the FMU input snapshot");
    Require(capture.count == 1U && capture.first_bytes[0] == 0xa5U, "XRCE payload was not demultiplexed");
    Require(DaqCoordinatorQueueXrce(&coordinator, outbound_xrce_payload, sizeof(outbound_xrce_payload)) == DAQ_COORDINATOR_OK,
            "could not queue outbound XRCE");
    Require(DaqCoordinatorQueueXrce(&coordinator, outbound_xrce_followup, sizeof(outbound_xrce_followup)) == DAQ_COORDINATOR_OK,
            "could not queue consecutive outbound XRCE datagram");
    Require(DaqCoordinatorTakeTransmit(&coordinator, outgoing, sizeof(outgoing), &frame) == DAQ_COORDINATOR_OK &&
            frame.kind == DAQ_TRANSMIT_READ_ACK && frame.size == 5U && outgoing[2] == DAQ_PROTOCOL_MID_READ_ACK && outgoing[3] == 9U,
            "READ_ACK was not prioritized after a consumed DATA frame");
    Require(DaqCoordinatorPublishOutput(&coordinator, 3U, output_payload, sizeof(output_payload)) == DAQ_COORDINATOR_OK,
            "streaming output was rejected");
    Require(DaqCoordinatorQueueCommand(&coordinator, DAQ_PROTOCOL_COMMAND_DISABLE) == DAQ_COORDINATOR_OK,
            "DISABLE command was not queued");
    Require(DaqCoordinatorTakeTransmit(&coordinator, outgoing, sizeof(outgoing), &frame) == DAQ_COORDINATOR_OK &&
            frame.kind == DAQ_TRANSMIT_CONFIG && outgoing[3] == DAQ_PROTOCOL_COMMAND_DISABLE,
            "CONFIG did not take priority over pending DATA");
    Require(DaqCoordinatorTakeTransmit(&coordinator, outgoing, sizeof(outgoing), &frame) == DAQ_COORDINATOR_OK &&
            frame.kind == DAQ_TRANSMIT_DATA && outgoing[2] == DAQ_PROTOCOL_MID_DATA,
            "DATA did not take priority over pending XRCE");
    Require(DaqCoordinatorTakeTransmit(&coordinator, outgoing, sizeof(outgoing), &frame) == DAQ_COORDINATOR_OK &&
            frame.kind == DAQ_TRANSMIT_XRCE && frame.size == 7U && outgoing[2] == DAQ_PROTOCOL_MID_XRCE && outgoing[5] == 0x42U,
            "first outbound XRCE datagram was not transmitted after critical traffic");
    Require(DaqCoordinatorTakeTransmit(&coordinator, outgoing, sizeof(outgoing), &frame) == DAQ_COORDINATOR_OK &&
            frame.kind == DAQ_TRANSMIT_XRCE && frame.size == 7U && outgoing[2] == DAQ_PROTOCOL_MID_XRCE && outgoing[5] == 0x43U,
            "second outbound XRCE datagram was not preserved in FIFO order");
    Require(DaqCoordinatorQueueXrce(&coordinator, outbound_xrce_payload, sizeof(outbound_xrce_payload)) == DAQ_COORDINATOR_OK,
            "could not queue XRCE before a state change");
    Require(DaqProtocolEncodeConfig(incoming, sizeof(incoming), DAQ_PROTOCOL_COMMAND_DISABLE, true, DAQ_PROTOCOL_COMMAND_DISABLE) == 5U &&
            DaqCoordinatorReceive(&coordinator, incoming, 5U) == DAQ_COORDINATOR_OK, "DISABLE confirmation was rejected");
    Require(DaqCoordinatorTakeTransmit(&coordinator, outgoing, sizeof(outgoing), &frame) == DAQ_COORDINATOR_OK &&
            frame.kind == DAQ_TRANSMIT_XRCE && outgoing[5] == 0x42U,
            "CONFIG confirmation discarded an XRCE datagram that belongs to the independent Agent session");
    for (size_t index = 0U; index < DAQ_COORDINATOR_XRCE_QUEUE_DEPTH; ++index) {
        Require(DaqCoordinatorQueueXrce(&coordinator, outbound_xrce_payload, sizeof(outbound_xrce_payload)) == DAQ_COORDINATOR_OK,
                "XRCE FIFO rejected a datagram before reaching its documented capacity");
    }
    Require(DaqCoordinatorQueueXrce(&coordinator, outbound_xrce_payload, sizeof(outbound_xrce_payload)) == DAQ_COORDINATOR_BUSY &&
            coordinator.xrce_queue_overflows == 1U,
            "XRCE FIFO saturation was not rejected and counted");
    DaqCoordinatorDestroy(&coordinator);
    InputStateDestroy(&input_state);
    return EXIT_SUCCESS;
}
