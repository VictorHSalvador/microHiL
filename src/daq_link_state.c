/* Implementação do módulo daq link state. */
#include "daq_link_state.h"

#include <string.h>

static daq_link_status_t Lock(daq_link_state_t *state) {
    return pthread_mutex_lock(&state->mutex) == 0 ? DAQ_LINK_OK : DAQ_LINK_MUTEX;
}

static daq_link_status_t Unlock(daq_link_state_t *state) {
    return pthread_mutex_unlock(&state->mutex) == 0 ? DAQ_LINK_OK : DAQ_LINK_MUTEX;
}

daq_link_status_t DaqLinkStateInit(daq_link_state_t *state) {
    if (!state) {
        return DAQ_LINK_INVALID_ARGUMENT;
    }
    *state = (daq_link_state_t){0};
    if (pthread_mutex_init(&state->mutex, NULL) != 0) {
        return DAQ_LINK_MUTEX;
    }
    state->initialized = true;
    return DAQ_LINK_OK;
}

void DaqLinkStateDestroy(daq_link_state_t *state) {
    if (!state || !state->initialized) {
        return;
    }
    (void)pthread_mutex_destroy(&state->mutex);
    *state = (daq_link_state_t){0};
}

daq_link_status_t DaqLinkStateConfirm(daq_link_state_t *state, daq_protocol_command_t command, uint8_t status) {
    if (!state || !state->initialized || command < DAQ_PROTOCOL_COMMAND_DISABLE || command > DAQ_PROTOCOL_COMMAND_STREAMING || status != (uint8_t)command) {
        return DAQ_LINK_INVALID_ARGUMENT;
    }
    const daq_link_status_t lock_status = Lock(state);
    if (lock_status != DAQ_LINK_OK) return lock_status;
    if (command == DAQ_PROTOCOL_COMMAND_DISABLE) {
        state->mode = DAQ_LINK_DISABLED;
        state->output_mailbox.pending = false;
        state->read_ack_pending = false;
        state->acquisition_sequence = (daq_sequence_tracker_t){0};
    } else if (command == DAQ_PROTOCOL_COMMAND_ENABLE) {
        state->mode = DAQ_LINK_ENABLED;
        state->output_mailbox.pending = false;
        state->read_ack_pending = false;
        state->acquisition_sequence = (daq_sequence_tracker_t){0};
    } else if (state->mode == DAQ_LINK_ENABLED || state->mode == DAQ_LINK_STREAMING) {
        state->mode = DAQ_LINK_STREAMING;
        state->acquisition_sequence = (daq_sequence_tracker_t){0};
    } else {
        (void)Unlock(state);
        return DAQ_LINK_INVALID_STATE;
    }
    return Unlock(state);
}

daq_link_status_t DaqLinkStateGetMode(daq_link_state_t *state, daq_link_mode_t *mode) {
    if (!state || !state->initialized || !mode) return DAQ_LINK_INVALID_ARGUMENT;
    const daq_link_status_t lock_status = Lock(state);
    if (lock_status != DAQ_LINK_OK) return lock_status;
    *mode = state->mode;
    return Unlock(state);
}

daq_link_status_t DaqLinkStatePublishOutput(daq_link_state_t *state, uint16_t sequence, const uint8_t *payload, size_t payload_size) {
    if (!state || !state->initialized || (payload_size > 0U && !payload) || payload_size > DAQ_PROTOCOL_MAX_DATA_PAYLOAD) {
        return DAQ_LINK_INVALID_ARGUMENT;
    }
    const daq_link_status_t lock_status = Lock(state);
    if (lock_status != DAQ_LINK_OK) return lock_status;
    if (state->mode != DAQ_LINK_STREAMING) {
        (void)Unlock(state);
        return DAQ_LINK_INVALID_STATE;
    }
    if (state->output_mailbox.pending) {
        ++state->output_coalesced;
    }
    if (payload_size > 0U) {
        memcpy(state->output_mailbox.payload, payload, payload_size);
    }
    state->output_mailbox.payload_size = payload_size;
    state->output_mailbox.sequence = sequence;
    state->output_mailbox.pending = true;
    return Unlock(state);
}

daq_link_status_t DaqLinkStateTakeOutput(daq_link_state_t *state, daq_output_mailbox_t *mailbox) {
    if (!state || !state->initialized || !mailbox) {
        return DAQ_LINK_INVALID_ARGUMENT;
    }
    const daq_link_status_t lock_status = Lock(state);
    if (lock_status != DAQ_LINK_OK) return lock_status;
    *mailbox = state->output_mailbox;
    state->output_mailbox.pending = false;
    return Unlock(state);
}

daq_link_status_t DaqLinkStateAcceptAcquisition(daq_link_state_t *state, uint16_t sequence, bool *accepted) {
    if (!state || !state->initialized || !accepted) {
        return DAQ_LINK_INVALID_ARGUMENT;
    }
    const daq_link_status_t lock_status = Lock(state);
    if (lock_status != DAQ_LINK_OK) return lock_status;
    if (state->mode != DAQ_LINK_STREAMING) {
        (void)Unlock(state);
        return DAQ_LINK_INVALID_STATE;
    }
    const daq_sequence_relation_t relation = DaqSequenceTrack(&state->acquisition_sequence, sequence);
    *accepted = relation == DAQ_SEQUENCE_FIRST || relation == DAQ_SEQUENCE_EXPECTED || relation == DAQ_SEQUENCE_GAP;
    if (*accepted) {
        state->latest_read_ack = sequence;
        state->read_ack_pending = true;
    }
    return Unlock(state);
}

daq_link_status_t DaqLinkStateTakeReadAck(daq_link_state_t *state, uint16_t *sequence, bool *available) {
    if (!state || !state->initialized || !sequence || !available) {
        return DAQ_LINK_INVALID_ARGUMENT;
    }
    const daq_link_status_t lock_status = Lock(state);
    if (lock_status != DAQ_LINK_OK) return lock_status;
    *available = state->read_ack_pending;
    *sequence = state->latest_read_ack;
    state->read_ack_pending = false;
    return Unlock(state);
}
