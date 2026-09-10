#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "daq_link_state.h"

static void Require(bool condition, const char *message) {
    if (!condition) {
        fprintf(stderr, "%s\n", message);
        exit(EXIT_FAILURE);
    }
}

int main(void) {
    const uint8_t first_payload[] = {1U, 2U};
    const uint8_t latest_payload[] = {3U, 4U, 5U};
    daq_link_state_t state;
    daq_output_mailbox_t mailbox;
    uint16_t acknowledged_sequence = 0U;
    bool accepted = false;
    bool acknowledgment_available = false;

    Require(DaqLinkStateInit(&state) == DAQ_LINK_OK, "could not initialize link state");
    Require(DaqLinkStatePublishOutput(&state, 1U, first_payload, sizeof(first_payload)) == DAQ_LINK_INVALID_STATE,
            "output was accepted outside STREAMING");
    Require(DaqLinkStateConfirm(&state, DAQ_PROTOCOL_COMMAND_ENABLE, DAQ_PROTOCOL_COMMAND_ENABLE) == DAQ_LINK_OK,
            "ENABLE confirmation was rejected");
    Require(DaqLinkStateConfirm(&state, DAQ_PROTOCOL_COMMAND_STREAMING, DAQ_PROTOCOL_COMMAND_STREAMING) == DAQ_LINK_OK,
            "STREAMING confirmation was rejected");
    Require(DaqLinkStatePublishOutput(&state, 1U, first_payload, sizeof(first_payload)) == DAQ_LINK_OK,
            "first output was rejected");
    Require(DaqLinkStatePublishOutput(&state, 2U, latest_payload, sizeof(latest_payload)) == DAQ_LINK_OK,
            "latest output was rejected");
    Require(DaqLinkStateTakeOutput(&state, &mailbox) == DAQ_LINK_OK && mailbox.pending && mailbox.sequence == 2U &&
            mailbox.payload_size == sizeof(latest_payload) && mailbox.payload[0] == latest_payload[0] && state.output_coalesced == 1U,
            "output mailbox did not retain the latest snapshot");
    Require(DaqLinkStateAcceptAcquisition(&state, 4U, &accepted) == DAQ_LINK_OK && accepted,
            "first acquisition was rejected");
    Require(DaqLinkStateAcceptAcquisition(&state, 6U, &accepted) == DAQ_LINK_OK && accepted,
            "acquisition gap was rejected");
    Require(state.acquisition_sequence.missing_frames == 1U, "acquisition gap was not counted");
    Require(DaqLinkStateAcceptAcquisition(&state, 6U, &accepted) == DAQ_LINK_OK && !accepted,
            "duplicate acquisition was accepted");
    Require(DaqLinkStateTakeReadAck(&state, &acknowledged_sequence, &acknowledgment_available) == DAQ_LINK_OK &&
            acknowledgment_available && acknowledged_sequence == 6U, "latest read acknowledgment was not retained");
    Require(DaqLinkStateConfirm(&state, DAQ_PROTOCOL_COMMAND_ENABLE, DAQ_PROTOCOL_COMMAND_ENABLE) == DAQ_LINK_OK,
            "ENABLE transition was rejected");
    Require(DaqLinkStateTakeReadAck(&state, &acknowledged_sequence, &acknowledgment_available) == DAQ_LINK_OK &&
            !acknowledgment_available, "ENABLE did not clear a pending read acknowledgment");
    Require(DaqLinkStateConfirm(&state, DAQ_PROTOCOL_COMMAND_DISABLE, DAQ_PROTOCOL_COMMAND_DISABLE) == DAQ_LINK_OK,
            "DISABLE confirmation was rejected");
    Require(DaqLinkStateTakeOutput(&state, &mailbox) == DAQ_LINK_OK && !mailbox.pending,
            "DISABLE did not clear the output mailbox");
    DaqLinkStateDestroy(&state);
    return EXIT_SUCCESS;
}
