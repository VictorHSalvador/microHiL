#ifndef DAQ_ACTUATION_H
#define DAQ_ACTUATION_H

#include <stdbool.h>

#include "daq_schema.h"
#include "daq_protocol.h"
#include "log_format.h"

typedef struct {
    const daq_schema_t *schema;
    uint8_t last_payload[DAQ_PROTOCOL_MAX_DATA_PAYLOAD];
    bool field_has_last_value[DAQ_SCHEMA_MAX_FIELDS];
    bool initialized;
} daq_actuation_t;

typedef enum {
    DAQ_ACTUATION_OK = 0,
    DAQ_ACTUATION_INVALID_ARGUMENT,
    DAQ_ACTUATION_INCOMPATIBLE_SCHEMA,
    DAQ_ACTUATION_OUTPUT_INVALID,
    DAQ_ACTUATION_OUTPUT_RANGE
} daq_actuation_status_t;

daq_actuation_status_t DaqActuationInit(daq_actuation_t *actuation, const daq_schema_t *schema);
daq_actuation_status_t DaqActuationPack(daq_actuation_t *actuation, const log_value_t *values, size_t value_count,
                                        uint8_t *payload, size_t payload_capacity);
void DaqActuationZero(const daq_actuation_t *actuation, uint8_t *payload, size_t payload_capacity);
const char *DaqActuationStatusString(daq_actuation_status_t status);

#endif
