#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "daq_state_control.h"

typedef struct {
    daq_coordinator_t *coordinator;
    daq_protocol_command_t command;
} confirmation_thread_t;

static void Require(bool condition, const char *message) {
    if (!condition) {
        fprintf(stderr, "%s\n", message);
        exit(EXIT_FAILURE);
    }
}

static void *ConfirmState(void *argument) {
    confirmation_thread_t *thread = argument;
    const struct timespec interval = {.tv_sec = 0, .tv_nsec = 10000000L};
    (void)nanosleep(&interval, NULL);
    uint8_t frame[5];
    Require(DaqProtocolEncodeConfig(frame, sizeof(frame), thread->command, true, (uint8_t)thread->command) == sizeof(frame),
            "could not encode DAQC confirmation");
    Require(DaqCoordinatorReceive(thread->coordinator, frame, sizeof(frame)) == DAQ_COORDINATOR_OK,
            "could not deliver DAQC confirmation");
    return NULL;
}

int main(void) {
    const input_channel_descriptor_t inputs[] = {
        {.input_name = "input", .value_reference = 1U, .type = NUMERIC_REAL, .initial_value = {.real_value = 0.0}, .initial_value_valid = true}
    };
    const daq_field_t fields[] = {
        {.gpio = 34U, .function = DAQ_CHANNEL_AI, .wire_type = DAQ_WIRE_FLOAT32, .fmu_type = NUMERIC_REAL,
         .offset = 0U, .width = 4U, .scale = 1.0, .offset_value = 0.0, .fmu_index = 0U}
    };
    daq_schema_t schema;
    input_state_t input_state;
    daq_coordinator_t coordinator;
    const daq_coordinator_config_t config = {
        .acquisition_schema = &schema, .input_state = &input_state, .xrce_mtu = DAQ_PROTOCOL_XRCE_MTU
    };
    confirmation_thread_t confirmation = {.coordinator = &coordinator, .command = DAQ_PROTOCOL_COMMAND_STREAMING};
    pthread_t thread;

    Require(DaqSchemaBuild(&schema, fields, 1U) == DAQ_SCHEMA_OK, "could not build acquisition schema");
    Require(InputStateInit(&input_state, inputs, 1U, false) == INPUT_STATE_STATUS_OK, "could not initialize input state");
    Require(DaqCoordinatorInit(&coordinator, &config) == DAQ_COORDINATOR_OK, "could not initialize coordinator");
    confirmation.command = DAQ_PROTOCOL_COMMAND_DISABLE;
    Require(pthread_create(&thread, NULL, ConfirmState, &confirmation) == 0, "could not create baseline disable confirmation thread");
    Require(DaqStateControlConfirmDisable(&coordinator, 100U) == DAQ_STATE_CONTROL_OK, "baseline DISABLE was not confirmed");
    Require(pthread_join(thread, NULL) == 0, "could not join baseline disable confirmation thread");
    Require(DaqLinkStateConfirm(&coordinator.link_state, DAQ_PROTOCOL_COMMAND_ENABLE, DAQ_PROTOCOL_COMMAND_ENABLE) == DAQ_LINK_OK,
            "could not establish ENABLE state");
    confirmation.command = DAQ_PROTOCOL_COMMAND_STREAMING;
    Require(pthread_create(&thread, NULL, ConfirmState, &confirmation) == 0, "could not create streaming confirmation thread");
    Require(DaqStateControlPlay(&coordinator, 100U) == DAQ_STATE_CONTROL_OK, "Play did not await ENABLE to STREAMING confirmation");
    Require(pthread_join(thread, NULL) == 0, "could not join streaming confirmation thread");

    confirmation.command = DAQ_PROTOCOL_COMMAND_DISABLE;
    Require(pthread_create(&thread, NULL, ConfirmState, &confirmation) == 0, "could not create disable confirmation thread");
    Require(DaqStateControlStop(&coordinator, 100U) == DAQ_STATE_CONTROL_OK, "Stop did not await STREAMING to DISABLE confirmation");
    Require(pthread_join(thread, NULL) == 0, "could not join disable confirmation thread");
    Require(DaqStateControlPlay(&coordinator, 1U) == DAQ_STATE_CONTROL_UNEXPECTED_STATE,
            "Play was accepted outside ENABLE");
    Require(DaqStateControlTransition(&coordinator, DAQ_LINK_DISABLED, DAQ_PROTOCOL_COMMAND_ENABLE, 1U) == DAQ_STATE_CONTROL_TIMEOUT,
            "missing CONFIG confirmation did not time out");

    DaqCoordinatorDestroy(&coordinator);
    InputStateDestroy(&input_state);
    return EXIT_SUCCESS;
}
