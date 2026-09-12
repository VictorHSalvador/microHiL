#ifndef DAQ_ACQUISITION_H
#define DAQ_ACQUISITION_H

/* Contrato público do módulo daq acquisition. */

#include <stdint.h>

#include "daq_schema.h"

typedef struct {
    const daq_schema_t *schema;
    input_state_t *input_state;
    uint64_t accepted_frames;
    uint64_t invalid_values;
    bool initialized;
} daq_acquisition_t;

typedef enum {
    DAQ_ACQUISITION_OK = 0,
    DAQ_ACQUISITION_INVALID_ARGUMENT,
    DAQ_ACQUISITION_INCOMPATIBLE_SCHEMA,
    DAQ_ACQUISITION_PAYLOAD_SIZE,
    DAQ_ACQUISITION_INPUT_STATE
} daq_acquisition_status_t;

daq_acquisition_status_t DaqAcquisitionInit(daq_acquisition_t *acquisition, const daq_schema_t *schema, input_state_t *input_state);
daq_acquisition_status_t DaqAcquisitionApply(daq_acquisition_t *acquisition, const uint8_t *payload, size_t payload_size);
const char *DaqAcquisitionStatusString(daq_acquisition_status_t status);

#endif
