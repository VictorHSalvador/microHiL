#ifndef INPUT_STATE_H
#define INPUT_STATE_H

/* Contrato público do módulo input state. */

#include <stdbool.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>

#include "common.h"

#define INPUT_STATE_MAX_CHANNELS MAX_OUTPUTS
#define INPUT_STATE_INVALID_STEP_LIMIT 100U

typedef union {
    double real_value;
    int64_t discrete_value;
    uint8_t boolean_value;
} input_value_t;

typedef struct {
    char input_name[OUTPUT_NAME_LEN];
    uint32_t value_reference;
    NumericType type;
    input_value_t initial_value;
    bool initial_value_valid;
} input_channel_descriptor_t;

typedef struct {
    input_value_t value;
    bool valid;
    uint64_t generation;
} input_candidate_t;

typedef struct {
    input_channel_descriptor_t descriptor;
    input_candidate_t candidate;
    input_value_t last_valid_value;
    bool has_last_valid_value;
    uint64_t consumed_generation;
    uint64_t consecutive_invalid_steps;
} input_channel_state_t;

typedef struct {
    pthread_mutex_t mutex;
    input_channel_state_t channels[INPUT_STATE_MAX_CHANNELS];
    size_t channel_count;
    bool stop_on_invalid_limit;
    bool initialized;
} input_state_t;

typedef struct {
    input_value_t values[INPUT_STATE_MAX_CHANNELS];
    size_t value_count;
    size_t invalid_limit_channels[INPUT_STATE_MAX_CHANNELS];
    size_t invalid_limit_channel_count;
    bool protection_tripped;
} input_step_t;

typedef enum {
    INPUT_STATE_STATUS_OK = 0,
    INPUT_STATE_STATUS_INVALID_ARGUMENT,
    INPUT_STATE_STATUS_NO_REFERENCE,
    INPUT_STATE_STATUS_PROTECTION_TRIPPED,
    INPUT_STATE_STATUS_MUTEX
} input_state_status_t;

input_state_status_t InputStateInit(input_state_t *state, const input_channel_descriptor_t *descriptors, size_t count, bool stop_on_invalid_limit);
void InputStateDestroy(input_state_t *state);
input_state_status_t InputStatePublish(input_state_t *state, size_t channel_index, input_value_t value, bool transport_valid);
input_state_status_t InputStatePrepareStep(input_state_t *state, input_step_t *step);
const char *InputStateStatusString(input_state_status_t status);

#endif
