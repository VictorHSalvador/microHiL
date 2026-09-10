#ifndef DAQ_COORDINATOR_H
#define DAQ_COORDINATOR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "daq_acquisition.h"
#include "daq_link_state.h"

typedef bool (*daq_xrce_receive_callback_t)(const uint8_t *payload, size_t payload_size, void *context);

typedef struct {
    const daq_schema_t *acquisition_schema;
    input_state_t *input_state;
    uint16_t xrce_mtu;
    daq_xrce_receive_callback_t xrce_receive;
    void *xrce_context;
} daq_coordinator_config_t;

typedef enum {
    DAQ_TRANSMIT_NONE = 0,
    DAQ_TRANSMIT_CONFIG,
    DAQ_TRANSMIT_READ_ACK,
    DAQ_TRANSMIT_DATA,
    DAQ_TRANSMIT_XRCE
} daq_transmit_kind_t;

typedef struct {
    daq_transmit_kind_t kind;
    size_t size;
} daq_transmit_frame_t;

typedef struct {
    daq_link_state_t link_state;
    daq_acquisition_t acquisition;
    daq_protocol_parser_t parser;
    uint8_t *parser_storage;
    size_t parser_storage_size;
    daq_xrce_receive_callback_t xrce_receive;
    void *xrce_context;
    pthread_mutex_t transmit_mutex;
    daq_protocol_command_t pending_command;
    bool command_pending;
    uint8_t xrce_mailbox[DAQ_PROTOCOL_XRCE_MTU];
    size_t xrce_mailbox_size;
    bool xrce_pending;
    uint64_t xrce_coalesced;
    uint64_t rejected_frames;
    uint64_t xrce_frames;
    bool initialized;
} daq_coordinator_t;

typedef enum {
    DAQ_COORDINATOR_OK = 0,
    DAQ_COORDINATOR_INVALID_ARGUMENT,
    DAQ_COORDINATOR_ALLOCATION,
    DAQ_COORDINATOR_PROTOCOL,
    DAQ_COORDINATOR_BUSY,
    DAQ_COORDINATOR_STATE,
    DAQ_COORDINATOR_MUTEX
} daq_coordinator_status_t;

daq_coordinator_status_t DaqCoordinatorInit(daq_coordinator_t *coordinator, const daq_coordinator_config_t *config);
void DaqCoordinatorDestroy(daq_coordinator_t *coordinator);
daq_coordinator_status_t DaqCoordinatorReceive(daq_coordinator_t *coordinator, const uint8_t *bytes, size_t byte_count);
daq_coordinator_status_t DaqCoordinatorQueueCommand(daq_coordinator_t *coordinator, daq_protocol_command_t command);
daq_coordinator_status_t DaqCoordinatorPublishOutput(daq_coordinator_t *coordinator, uint16_t sequence, const uint8_t *payload, size_t payload_size);
daq_coordinator_status_t DaqCoordinatorQueueXrce(daq_coordinator_t *coordinator, const uint8_t *payload, size_t payload_size);
daq_coordinator_status_t DaqCoordinatorTakeTransmit(daq_coordinator_t *coordinator, uint8_t *destination, size_t capacity,
                                                     daq_transmit_frame_t *frame);
const char *DaqCoordinatorStatusString(daq_coordinator_status_t status);

#endif
