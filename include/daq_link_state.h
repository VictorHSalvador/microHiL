#ifndef DAQ_LINK_STATE_H
#define DAQ_LINK_STATE_H

#include <stdbool.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>

#include "daq_protocol.h"

typedef enum {
    DAQ_LINK_DISABLED = 0,
    DAQ_LINK_ENABLED,
    DAQ_LINK_STREAMING
} daq_link_mode_t;

typedef struct {
    uint8_t payload[DAQ_PROTOCOL_MAX_DATA_PAYLOAD];
    size_t payload_size;
    uint16_t sequence;
    bool pending;
} daq_output_mailbox_t;

typedef struct {
    pthread_mutex_t mutex;
    daq_link_mode_t mode;
    daq_output_mailbox_t output_mailbox;
    daq_sequence_tracker_t acquisition_sequence;
    uint16_t latest_read_ack;
    bool read_ack_pending;
    uint64_t output_coalesced;
    bool initialized;
} daq_link_state_t;

typedef enum {
    DAQ_LINK_OK = 0,
    DAQ_LINK_INVALID_ARGUMENT,
    DAQ_LINK_INVALID_STATE,
    DAQ_LINK_MUTEX
} daq_link_status_t;

daq_link_status_t DaqLinkStateInit(daq_link_state_t *state);
void DaqLinkStateDestroy(daq_link_state_t *state);
daq_link_status_t DaqLinkStateConfirm(daq_link_state_t *state, daq_protocol_command_t command, uint8_t status);
daq_link_status_t DaqLinkStateGetMode(daq_link_state_t *state, daq_link_mode_t *mode);
daq_link_status_t DaqLinkStatePublishOutput(daq_link_state_t *state, uint16_t sequence, const uint8_t *payload, size_t payload_size);
daq_link_status_t DaqLinkStateTakeOutput(daq_link_state_t *state, daq_output_mailbox_t *mailbox);
daq_link_status_t DaqLinkStateAcceptAcquisition(daq_link_state_t *state, uint16_t sequence, bool *accepted);
daq_link_status_t DaqLinkStateTakeReadAck(daq_link_state_t *state, uint16_t *sequence, bool *available);

#endif
