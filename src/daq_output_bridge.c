/* Implementação do módulo daq output bridge. */
#include "daq_output_bridge.h"

static daq_output_bridge_status_t PublishPayload(daq_output_bridge_t *bridge, const uint8_t *payload) {
    const daq_coordinator_status_t status = DaqCoordinatorPublishOutput(bridge->coordinator, bridge->next_sequence, payload,
                                                                         bridge->actuation.schema->payload_size);
    if (status != DAQ_COORDINATOR_OK) return DAQ_OUTPUT_BRIDGE_COORDINATOR;
    ++bridge->next_sequence;
    return DAQ_OUTPUT_BRIDGE_OK;
}

daq_output_bridge_status_t DaqOutputBridgeInit(daq_output_bridge_t *bridge, const daq_schema_t *schema, daq_coordinator_t *coordinator) {
    if (!bridge || !schema || !coordinator || !coordinator->initialized) return DAQ_OUTPUT_BRIDGE_INVALID_ARGUMENT;
    *bridge = (daq_output_bridge_t){.coordinator = coordinator};
    if (DaqActuationInit(&bridge->actuation, schema) != DAQ_ACTUATION_OK) return DAQ_OUTPUT_BRIDGE_ACTUATION;
    bridge->initialized = true;
    return DAQ_OUTPUT_BRIDGE_OK;
}

daq_output_bridge_status_t DaqOutputBridgePublish(daq_output_bridge_t *bridge, const OutputVariable *outputs, const log_value_t *values,
                                                   size_t value_count) {
    if (!bridge || !bridge->initialized) return DAQ_OUTPUT_BRIDGE_INVALID_ARGUMENT;
    uint8_t payload[DAQ_PROTOCOL_MAX_DATA_PAYLOAD];
    if (DaqActuationPack(&bridge->actuation, outputs, values, value_count, payload, sizeof(payload)) != DAQ_ACTUATION_OK) {
        return DAQ_OUTPUT_BRIDGE_ACTUATION;
    }
    return PublishPayload(bridge, payload);
}

daq_output_bridge_status_t DaqOutputBridgePublishZero(daq_output_bridge_t *bridge) {
    if (!bridge || !bridge->initialized) return DAQ_OUTPUT_BRIDGE_INVALID_ARGUMENT;
    uint8_t payload[DAQ_PROTOCOL_MAX_DATA_PAYLOAD];
    DaqActuationZero(&bridge->actuation, payload, sizeof(payload));
    return PublishPayload(bridge, payload);
}

const char *DaqOutputBridgeStatusString(daq_output_bridge_status_t status) {
    switch (status) {
        case DAQ_OUTPUT_BRIDGE_OK: return "ok";
        case DAQ_OUTPUT_BRIDGE_INVALID_ARGUMENT: return "invalid argument";
        case DAQ_OUTPUT_BRIDGE_ACTUATION: return "could not encode FMU output";
        case DAQ_OUTPUT_BRIDGE_COORDINATOR: return "DAQ link did not accept the output";
        default: return "unknown output bridge status";
    }
}
