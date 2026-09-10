#ifndef DAQ_OUTPUT_BRIDGE_H
#define DAQ_OUTPUT_BRIDGE_H

#include "daq_actuation.h"
#include "daq_coordinator.h"

typedef struct {
    daq_actuation_t actuation;
    daq_coordinator_t *coordinator;
    uint16_t next_sequence;
    bool initialized;
} daq_output_bridge_t;

typedef enum {
    DAQ_OUTPUT_BRIDGE_OK = 0,
    DAQ_OUTPUT_BRIDGE_INVALID_ARGUMENT,
    DAQ_OUTPUT_BRIDGE_ACTUATION,
    DAQ_OUTPUT_BRIDGE_COORDINATOR
} daq_output_bridge_status_t;

daq_output_bridge_status_t DaqOutputBridgeInit(daq_output_bridge_t *bridge, const daq_schema_t *schema, daq_coordinator_t *coordinator);
daq_output_bridge_status_t DaqOutputBridgePublish(daq_output_bridge_t *bridge, const OutputVariable *outputs, const log_value_t *values,
                                                   size_t value_count);
daq_output_bridge_status_t DaqOutputBridgePublishZero(daq_output_bridge_t *bridge);
const char *DaqOutputBridgeStatusString(daq_output_bridge_status_t status);

#endif
