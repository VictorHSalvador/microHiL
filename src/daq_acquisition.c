/* Implementação do módulo daq acquisition. */
#include "daq_acquisition.h"

static bool IsInputFunction(daq_channel_function_t function) {
    return function == DAQ_CHANNEL_AI || function == DAQ_CHANNEL_DI;
}

daq_acquisition_status_t DaqAcquisitionInit(daq_acquisition_t *acquisition, const daq_schema_t *schema, input_state_t *input_state) {
    if (!acquisition || !schema || !input_state || !input_state->initialized) {
        return DAQ_ACQUISITION_INVALID_ARGUMENT;
    }
    for (size_t index = 0U; index < schema->field_count; ++index) {
        const daq_field_t *field = &schema->fields[index];
        if (!IsInputFunction(field->function) || field->fmu_index >= input_state->channel_count ||
            input_state->channels[field->fmu_index].descriptor.type != field->fmu_type) {
            return DAQ_ACQUISITION_INCOMPATIBLE_SCHEMA;
        }
    }
    *acquisition = (daq_acquisition_t){.schema = schema, .input_state = input_state, .initialized = true};
    return DAQ_ACQUISITION_OK;
}

daq_acquisition_status_t DaqAcquisitionApply(daq_acquisition_t *acquisition, const uint8_t *payload, size_t payload_size) {
    if (!acquisition || !acquisition->initialized || (payload_size > 0U && !payload)) {
        return DAQ_ACQUISITION_INVALID_ARGUMENT;
    }
    if (payload_size != acquisition->schema->payload_size) {
        return DAQ_ACQUISITION_PAYLOAD_SIZE;
    }
    for (size_t index = 0U; index < acquisition->schema->field_count; ++index) {
        const daq_field_t *field = &acquisition->schema->fields[index];
        input_value_t value;
        bool valid = false;
        if (DaqSchemaDecodeInput(field, payload, payload_size, &value, &valid) != DAQ_SCHEMA_OK ||
            InputStatePublish(acquisition->input_state, field->fmu_index, value, valid) != INPUT_STATE_STATUS_OK) {
            return DAQ_ACQUISITION_INPUT_STATE;
        }
        if (!valid) {
            ++acquisition->invalid_values;
        }
    }
    ++acquisition->accepted_frames;
    return DAQ_ACQUISITION_OK;
}

const char *DaqAcquisitionStatusString(daq_acquisition_status_t status) {
    switch (status) {
        case DAQ_ACQUISITION_OK:
            return "ok";
        case DAQ_ACQUISITION_INVALID_ARGUMENT:
            return "invalid argument";
        case DAQ_ACQUISITION_INCOMPATIBLE_SCHEMA:
            return "schema is incompatible with FMU inputs";
        case DAQ_ACQUISITION_PAYLOAD_SIZE:
            return "unexpected acquisition payload size";
        case DAQ_ACQUISITION_INPUT_STATE:
            return "could not publish acquisition input";
        default:
            return "unknown acquisition status";
    }
}
