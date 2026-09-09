#include "input_state.h"

#include <limits.h>
#include <math.h>
#include <string.h>

static bool IsValueValid(NumericType type, input_value_t value, bool transport_valid) {
    if (!transport_valid) return false;

    switch (type) {
        case NUMERIC_REAL: return isfinite(value.real_value);
        case NUMERIC_INTEGER:
        case NUMERIC_ENUMERATION: return value.discrete_value >= INT32_MIN && value.discrete_value <= INT32_MAX;
        case NUMERIC_BOOLEAN: return value.boolean_value <= UINT8_C(1);
        default: return false;
    }
}

input_state_status_t InputStateInit(input_state_t *state, const input_channel_descriptor_t *descriptors, size_t count, bool stop_on_invalid_limit) {
    if (!state || (count > 0U && !descriptors) || count > INPUT_STATE_MAX_CHANNELS) return INPUT_STATE_STATUS_INVALID_ARGUMENT;

    *state = (input_state_t){0};
    if (pthread_mutex_init(&state->mutex, NULL) != 0) return INPUT_STATE_STATUS_MUTEX;
    for (size_t index = 0U; index < count; ++index) {
        if (!descriptors[index].input_name[0] || !descriptors[index].initial_value_valid ||
            !IsValueValid(descriptors[index].type, descriptors[index].initial_value, true)) {
            (void)pthread_mutex_destroy(&state->mutex);
            *state = (input_state_t){0};
            return INPUT_STATE_STATUS_NO_REFERENCE;
        }
        state->channels[index].descriptor = descriptors[index];
    }
    state->channel_count = count;
    state->stop_on_invalid_limit = stop_on_invalid_limit;
    state->initialized = true;
    return INPUT_STATE_STATUS_OK;
}

void InputStateDestroy(input_state_t *state) {
    if (!state || !state->initialized) return;
    (void)pthread_mutex_destroy(&state->mutex);
    *state = (input_state_t){0};
}

input_state_status_t InputStatePublish(input_state_t *state, size_t channel_index, input_value_t value, bool transport_valid) {
    if (!state || !state->initialized || channel_index >= state->channel_count) return INPUT_STATE_STATUS_INVALID_ARGUMENT;
    if (pthread_mutex_lock(&state->mutex) != 0) return INPUT_STATE_STATUS_MUTEX;

    input_channel_state_t *channel = &state->channels[channel_index];
    channel->candidate.value = value;
    channel->candidate.valid = IsValueValid(channel->descriptor.type, value, transport_valid);
    ++channel->candidate.generation;

    if (pthread_mutex_unlock(&state->mutex) != 0) return INPUT_STATE_STATUS_MUTEX;
    return INPUT_STATE_STATUS_OK;
}

input_state_status_t InputStatePrepareStep(input_state_t *state, input_step_t *step) {
    if (!state || !state->initialized || !step) return INPUT_STATE_STATUS_INVALID_ARGUMENT;
    if (pthread_mutex_lock(&state->mutex) != 0) return INPUT_STATE_STATUS_MUTEX;

    *step = (input_step_t){.value_count = state->channel_count};
    for (size_t index = 0U; index < state->channel_count; ++index) {
        input_channel_state_t *channel = &state->channels[index];
        if (channel->candidate.generation != channel->consumed_generation) {
            channel->consumed_generation = channel->candidate.generation;
            if (channel->candidate.valid) {
                channel->last_valid_value = channel->candidate.value;
                channel->has_last_valid_value = true;
                channel->consecutive_invalid_steps = 0U;
            } else {
                ++channel->consecutive_invalid_steps;
                if (state->stop_on_invalid_limit && channel->consecutive_invalid_steps >= INPUT_STATE_INVALID_STEP_LIMIT) {
                    step->invalid_limit_channels[step->invalid_limit_channel_count++] = index;
                }
            }
        }

        if (channel->has_last_valid_value) {
            step->values[index] = channel->last_valid_value;
        } else if (channel->descriptor.initial_value_valid) {
            step->values[index] = channel->descriptor.initial_value;
        } else {
            (void)pthread_mutex_unlock(&state->mutex);
            return INPUT_STATE_STATUS_NO_REFERENCE;
        }
    }
    step->protection_tripped = step->invalid_limit_channel_count > 0U;

    if (pthread_mutex_unlock(&state->mutex) != 0) return INPUT_STATE_STATUS_MUTEX;
    return step->protection_tripped ? INPUT_STATE_STATUS_PROTECTION_TRIPPED : INPUT_STATE_STATUS_OK;
}

const char *InputStateStatusString(input_state_status_t status) {
    switch (status) {
        case INPUT_STATE_STATUS_OK: return "ok";
        case INPUT_STATE_STATUS_INVALID_ARGUMENT: return "invalid argument";
        case INPUT_STATE_STATUS_NO_REFERENCE: return "no valid initial input reference";
        case INPUT_STATE_STATUS_PROTECTION_TRIPPED: return "invalid input limit reached";
        case INPUT_STATE_STATUS_MUTEX: return "input state mutex failure";
        default: return "unknown input state status";
    }
}
