#include "daq_coordinator.h"

#include <stdlib.h>
#include <string.h>

static bool ProcessFrame(const daq_protocol_frame_t *frame, void *context) {
    daq_coordinator_t *coordinator = context;
    if (frame->mid == DAQ_PROTOCOL_MID_CONFIG) {
        if (!frame->has_status || DaqLinkStateConfirm(&coordinator->link_state, (daq_protocol_command_t)frame->command, frame->status) != DAQ_LINK_OK) {
            ++coordinator->rejected_frames;
        }
        return true;
    }
    if (frame->mid == DAQ_PROTOCOL_MID_DATA) {
        bool accepted = false;
        if (DaqLinkStateAcceptAcquisition(&coordinator->link_state, frame->sequence, &accepted) != DAQ_LINK_OK || !accepted) {
            ++coordinator->rejected_frames;
            return true;
        }
        if (DaqAcquisitionApply(&coordinator->acquisition, frame->payload, frame->payload_size) != DAQ_ACQUISITION_OK) {
            ++coordinator->rejected_frames;
        }
        return true;
    }
    if (frame->mid == DAQ_PROTOCOL_MID_XRCE) {
        ++coordinator->xrce_frames;
        if (coordinator->xrce_receive && !coordinator->xrce_receive(frame->payload, frame->payload_size, coordinator->xrce_context)) {
            ++coordinator->rejected_frames;
        }
        return true;
    }
    ++coordinator->rejected_frames;
    return true;
}

static bool IsCommandValid(daq_protocol_command_t command) {
    return command >= DAQ_PROTOCOL_COMMAND_DISABLE && command <= DAQ_PROTOCOL_COMMAND_STREAMING;
}

daq_coordinator_status_t DaqCoordinatorInit(daq_coordinator_t *coordinator, const daq_coordinator_config_t *config) {
    if (!coordinator || !config || !config->acquisition_schema || !config->input_state || config->xrce_mtu == 0U) {
        return DAQ_COORDINATOR_INVALID_ARGUMENT;
    }
    *coordinator = (daq_coordinator_t){0};
    if (DaqAcquisitionInit(&coordinator->acquisition, config->acquisition_schema, config->input_state) != DAQ_ACQUISITION_OK ||
        DaqLinkStateInit(&coordinator->link_state) != DAQ_LINK_OK) {
        return DAQ_COORDINATOR_STATE;
    }
    const size_t largest_payload = config->acquisition_schema->payload_size > config->xrce_mtu ? config->acquisition_schema->payload_size : config->xrce_mtu;
    coordinator->parser_storage_size = DAQ_PROTOCOL_DATA_PREFIX_SIZE + largest_payload;
    coordinator->parser_storage = calloc(coordinator->parser_storage_size, sizeof(coordinator->parser_storage[0]));
    if (!coordinator->parser_storage ||
        !DaqProtocolParserInit(&coordinator->parser, coordinator->parser_storage, coordinator->parser_storage_size,
                               config->acquisition_schema->payload_size, true)) {
        free(coordinator->parser_storage);
        DaqLinkStateDestroy(&coordinator->link_state);
        *coordinator = (daq_coordinator_t){0};
        return DAQ_COORDINATOR_ALLOCATION;
    }
    if (pthread_mutex_init(&coordinator->transmit_mutex, NULL) != 0) {
        free(coordinator->parser_storage);
        DaqLinkStateDestroy(&coordinator->link_state);
        *coordinator = (daq_coordinator_t){0};
        return DAQ_COORDINATOR_MUTEX;
    }
    coordinator->xrce_receive = config->xrce_receive;
    coordinator->xrce_context = config->xrce_context;
    coordinator->initialized = true;
    return DAQ_COORDINATOR_OK;
}

void DaqCoordinatorDestroy(daq_coordinator_t *coordinator) {
    if (!coordinator || !coordinator->initialized) {
        return;
    }
    (void)pthread_mutex_destroy(&coordinator->transmit_mutex);
    DaqLinkStateDestroy(&coordinator->link_state);
    free(coordinator->parser_storage);
    *coordinator = (daq_coordinator_t){0};
}

daq_coordinator_status_t DaqCoordinatorReceive(daq_coordinator_t *coordinator, const uint8_t *bytes, size_t byte_count) {
    if (!coordinator || !coordinator->initialized || (byte_count > 0U && !bytes)) {
        return DAQ_COORDINATOR_INVALID_ARGUMENT;
    }
    return DaqProtocolParserFeed(&coordinator->parser, bytes, byte_count, ProcessFrame, coordinator) == DAQ_PROTOCOL_PARSE_OK
        ? DAQ_COORDINATOR_OK : DAQ_COORDINATOR_PROTOCOL;
}

daq_coordinator_status_t DaqCoordinatorQueueCommand(daq_coordinator_t *coordinator, daq_protocol_command_t command) {
    if (!coordinator || !coordinator->initialized || !IsCommandValid(command)) {
        return DAQ_COORDINATOR_INVALID_ARGUMENT;
    }
    if (pthread_mutex_lock(&coordinator->transmit_mutex) != 0) {
        return DAQ_COORDINATOR_MUTEX;
    }
    if (coordinator->command_pending) {
        (void)pthread_mutex_unlock(&coordinator->transmit_mutex);
        return DAQ_COORDINATOR_BUSY;
    }
    coordinator->pending_command = command;
    coordinator->command_pending = true;
    return pthread_mutex_unlock(&coordinator->transmit_mutex) == 0 ? DAQ_COORDINATOR_OK : DAQ_COORDINATOR_MUTEX;
}

daq_coordinator_status_t DaqCoordinatorPublishOutput(daq_coordinator_t *coordinator, uint16_t sequence, const uint8_t *payload, size_t payload_size) {
    if (!coordinator || !coordinator->initialized) {
        return DAQ_COORDINATOR_INVALID_ARGUMENT;
    }
    const daq_link_status_t status = DaqLinkStatePublishOutput(&coordinator->link_state, sequence, payload, payload_size);
    if (status == DAQ_LINK_OK) {
        return DAQ_COORDINATOR_OK;
    }
    return status == DAQ_LINK_INVALID_STATE ? DAQ_COORDINATOR_STATE : DAQ_COORDINATOR_INVALID_ARGUMENT;
}

daq_coordinator_status_t DaqCoordinatorTakeTransmit(daq_coordinator_t *coordinator, uint8_t *destination, size_t capacity,
                                                     daq_transmit_frame_t *frame) {
    if (!coordinator || !coordinator->initialized || !destination || !frame) {
        return DAQ_COORDINATOR_INVALID_ARGUMENT;
    }
    if (capacity < DAQ_PROTOCOL_DATA_PREFIX_SIZE + DAQ_PROTOCOL_MAX_DATA_PAYLOAD) {
        return DAQ_COORDINATOR_INVALID_ARGUMENT;
    }
    *frame = (daq_transmit_frame_t){0};
    if (pthread_mutex_lock(&coordinator->transmit_mutex) != 0) {
        return DAQ_COORDINATOR_MUTEX;
    }
    if (coordinator->command_pending) {
        frame->size = DaqProtocolEncodeConfig(destination, capacity, coordinator->pending_command, false, 0U);
        frame->kind = DAQ_TRANSMIT_CONFIG;
        if (frame->size > 0U) {
            coordinator->command_pending = false;
        }
        if (pthread_mutex_unlock(&coordinator->transmit_mutex) != 0) {
            return DAQ_COORDINATOR_MUTEX;
        }
        return frame->size > 0U ? DAQ_COORDINATOR_OK : DAQ_COORDINATOR_INVALID_ARGUMENT;
    }
    if (pthread_mutex_unlock(&coordinator->transmit_mutex) != 0) {
        return DAQ_COORDINATOR_MUTEX;
    }
    uint16_t acknowledged_sequence = 0U;
    bool acknowledgement_available = false;
    if (DaqLinkStateTakeReadAck(&coordinator->link_state, &acknowledged_sequence, &acknowledgement_available) != DAQ_LINK_OK) {
        return DAQ_COORDINATOR_STATE;
    }
    if (acknowledgement_available) {
        frame->size = DaqProtocolEncodeReadAck(destination, capacity, acknowledged_sequence);
        frame->kind = DAQ_TRANSMIT_READ_ACK;
        return frame->size > 0U ? DAQ_COORDINATOR_OK : DAQ_COORDINATOR_INVALID_ARGUMENT;
    }
    daq_output_mailbox_t mailbox;
    if (DaqLinkStateTakeOutput(&coordinator->link_state, &mailbox) != DAQ_LINK_OK) {
        return DAQ_COORDINATOR_STATE;
    }
    if (!mailbox.pending) {
        return DAQ_COORDINATOR_OK;
    }
    frame->size = DaqProtocolEncodeData(destination, capacity, mailbox.sequence, mailbox.payload, mailbox.payload_size);
    frame->kind = DAQ_TRANSMIT_DATA;
    return frame->size > 0U ? DAQ_COORDINATOR_OK : DAQ_COORDINATOR_INVALID_ARGUMENT;
}

const char *DaqCoordinatorStatusString(daq_coordinator_status_t status) {
    switch (status) {
        case DAQ_COORDINATOR_OK:
            return "ok";
        case DAQ_COORDINATOR_INVALID_ARGUMENT:
            return "invalid argument";
        case DAQ_COORDINATOR_ALLOCATION:
            return "could not allocate coordinator parser storage";
        case DAQ_COORDINATOR_PROTOCOL:
            return "protocol parser failed";
        case DAQ_COORDINATOR_BUSY:
            return "a control command is already pending";
        case DAQ_COORDINATOR_STATE:
            return "DAQ link state rejected the operation";
        case DAQ_COORDINATOR_MUTEX:
            return "coordinator mutex failed";
        default:
            return "unknown coordinator status";
    }
}
