/* Implementação do módulo fmu model. */
#include "fmu_model.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void import_logger(jm_callbacks *callbacks, jm_string module, jm_log_level_enu_t level, jm_string message) {
    (void)callbacks;
    if (level <= jm_log_level_warning) fprintf(stderr, "[FMIL][%s][%s] %s\n", module, jm_log_level_to_string(level), message);
}

static bool status_is_ok(fmi2_status_t status) {
    return status == fmi2_status_ok || status == fmi2_status_warning;
}

static NumericType map_numeric_type(fmi2_base_type_enu_t type) {
    switch (type) {
        case fmi2_base_type_real: return NUMERIC_REAL;
        case fmi2_base_type_int: return NUMERIC_INTEGER;
        case fmi2_base_type_bool: return NUMERIC_BOOLEAN;
        case fmi2_base_type_enum: return NUMERIC_ENUMERATION;
        default: return NUMERIC_REAL;
    }
}

void fmu_model_init(FmuModel *model) {
    memset(model, 0, sizeof(*model));
    model->callbacks.malloc = malloc;
    model->callbacks.calloc = calloc;
    model->callbacks.realloc = realloc;
    model->callbacks.free = free;
    model->callbacks.logger = import_logger;
    model->callbacks.log_level = jm_log_level_warning;
    model->callbacks.context = NULL;
}

int fmu_model_load(FmuModel *model, const char *fmu_path) {
    fmi_version_enu_t version;
    fmi2_fmu_kind_enu_t kind;
    fmi2_callback_functions_t fmi_callbacks;
    jm_status_enu_t status;

    fmu_model_unload(model);
    fmu_model_init(model);

    model->tmp_path = fmi_import_mk_temp_dir(&model->callbacks, NULL, NULL);
    if (!model->tmp_path) {
        fprintf(stderr, "Could not create a temporary directory for FMU extraction.\n");
        return -1;
    }

    model->context = fmi_import_allocate_context(&model->callbacks);
    if (!model->context) {
        fprintf(stderr, "Could not allocate FMI import context.\n");
        fmu_model_unload(model);
        return -1;
    }

    version = fmi_import_get_fmi_version(model->context, fmu_path, model->tmp_path);
    if (version != fmi_version_2_0_enu) {
        fprintf(stderr, "The selected FMU is not FMI 2.0.\n");
        fmu_model_unload(model);
        return -1;
    }

    model->fmu = fmi2_import_parse_xml(model->context, model->tmp_path, 0);
    if (!model->fmu) {
        fprintf(stderr, "Could not parse modelDescription.xml.\n");
        fmu_model_unload(model);
        return -1;
    }

    kind = fmi2_import_get_fmu_kind(model->fmu);
    if (kind != fmi2_fmu_kind_cs && kind != fmi2_fmu_kind_me_and_cs) {
        fprintf(stderr, "The FMU does not provide an FMI 2.0 Co-Simulation interface.\n");
        fmu_model_unload(model);
        return -1;
    }

    snprintf(model->model_name, sizeof(model->model_name), "%s", fmi2_import_get_model_name(model->fmu));

    memset(&fmi_callbacks, 0, sizeof(fmi_callbacks));
    fmi_callbacks.logger = fmi2_log_forwarding;
    fmi_callbacks.allocateMemory = calloc;
    fmi_callbacks.freeMemory = free;
    fmi_callbacks.componentEnvironment = model->fmu;

    status = fmi2_import_create_dllfmu(model->fmu, fmi2_fmu_kind_cs, &fmi_callbacks);
    if (status == jm_status_error) {
        fprintf(stderr, "Could not load the FMU binary: %s\n", fmi2_import_get_last_error(model->fmu));
        fmu_model_unload(model);
        return -1;
    }

    model->dll_created = true;
    return 0;
}

void fmu_model_unload(FmuModel *model) {
    if (!model) return;

    if (model->instantiated) fmu_model_terminate(model);
    if (model->fmu && model->dll_created) {
        fmi2_import_destroy_dllfmu(model->fmu);
        model->dll_created = false;
    }
    if (model->fmu) {
        fmi2_import_free(model->fmu);
        model->fmu = NULL;
    }
    if (model->context) {
        fmi_import_free_context(model->context);
        model->context = NULL;
    }
    if (model->tmp_path) {
        fmi_import_rmdir(&model->callbacks, model->tmp_path);
        model->callbacks.free((void *)model->tmp_path);
        model->tmp_path = NULL;
    }
    model->model_name[0] = '\0';
}

size_t fmu_model_list_numeric_outputs(FmuModel *model, OutputVariable *outputs, size_t capacity) {
    if (!model || !model->fmu) return 0;

    fmi2_import_variable_list_t *list = fmi2_import_get_variable_list(model->fmu, 0);
    if (!list) return 0;

    size_t count = 0;
    size_t total = fmi2_import_get_variable_list_size(list);

    for (size_t i = 0; i < total; ++i) {
        fmi2_import_variable_t *var = fmi2_import_get_variable(list, i);
        fmi2_base_type_enu_t type = fmi2_import_get_variable_base_type(var);
        if (fmi2_import_get_causality(var) != fmi2_causality_enu_output) continue;
        if (type == fmi2_base_type_str) continue;

        if (outputs && count < capacity) {
            snprintf(outputs[count].name, sizeof(outputs[count].name), "%s", fmi2_import_get_variable_name(var));
            outputs[count].xml_index = (uint32_t)(i + 1U);
            outputs[count].value_reference = (uint32_t)fmi2_import_get_variable_vr(var);
            outputs[count].fmu_index = (uint16_t)count;
            outputs[count].type = map_numeric_type(type);
        }
        ++count;
    }

    fmi2_import_free_variable_list(list);
    return count;
}

size_t fmu_model_list_numeric_inputs(FmuModel *model, input_channel_descriptor_t *inputs, size_t capacity) {
    if (!model || !model->fmu) return 0U;

    fmi2_import_variable_list_t *list = fmi2_import_get_variable_list(model->fmu, 0);
    if (!list) return 0U;

    size_t count = 0U;
    const size_t total = fmi2_import_get_variable_list_size(list);
    for (size_t index = 0U; index < total; ++index) {
        fmi2_import_variable_t *variable = fmi2_import_get_variable(list, index);
        const fmi2_base_type_enu_t type = fmi2_import_get_variable_base_type(variable);
        if (fmi2_import_get_causality(variable) != fmi2_causality_enu_input || type == fmi2_base_type_str) continue;

        if (inputs && count < capacity) {
            input_channel_descriptor_t *input = &inputs[count];
            *input = (input_channel_descriptor_t){0};
            snprintf(input->input_name, sizeof(input->input_name), "%s", fmi2_import_get_variable_name(variable));
            input->value_reference = (uint32_t)fmi2_import_get_variable_vr(variable);
            input->type = map_numeric_type(type);
            if (fmi2_import_get_variable_has_start(variable)) {
                switch (input->type) {
                    case NUMERIC_REAL:
                        input->initial_value.real_value = (double)fmi2_import_get_real_variable_start(fmi2_import_get_variable_as_real(variable));
                        break;
                    case NUMERIC_INTEGER:
                        input->initial_value.discrete_value = fmi2_import_get_integer_variable_start(fmi2_import_get_variable_as_integer(variable));
                        break;
                    case NUMERIC_ENUMERATION:
                        input->initial_value.discrete_value = fmi2_import_get_enum_variable_start(fmi2_import_get_variable_as_enum(variable));
                        break;
                    case NUMERIC_BOOLEAN:
                        input->initial_value.boolean_value = fmi2_import_get_boolean_variable_start(fmi2_import_get_variable_as_boolean(variable)) == fmi2_true ? 1U : 0U;
                        break;
                    default:
                        break;
                }
                input->initial_value_valid = true;
            }
        }
        ++count;
    }

    fmi2_import_free_variable_list(list);
    return count;
}

int fmu_model_resolve_profile_mappings(FmuModel *model, profile_config_t *config) {
    if (!model || !model->fmu || !config) return -1;
    input_channel_descriptor_t inputs[INPUT_STATE_MAX_CHANNELS];
    OutputVariable outputs[MAX_OUTPUTS];
    const size_t input_count = fmu_model_list_numeric_inputs(model, inputs, INPUT_STATE_MAX_CHANNELS);
    const size_t output_count = fmu_model_list_numeric_outputs(model, outputs, MAX_OUTPUTS);
    for (size_t index = 0U; index < config->mapping_count; ++index) {
        profile_mapping_t *mapping = &config->mappings[index];
        mapping->is_input = strstr(mapping->channel, "_AI") != NULL || strstr(mapping->channel, "_DI") != NULL;
        bool found = false;
        if (mapping->is_input) {
            for (size_t candidate = 0U; candidate < input_count; ++candidate) {
                if (inputs[candidate].type == mapping->type && strcmp(inputs[candidate].input_name, mapping->variable) == 0) {
                    mapping->value_reference = inputs[candidate].value_reference;
                    mapping->fmu_index = (uint16_t)candidate;
                    found = true;
                    break;
                }
            }
        } else {
            for (size_t candidate = 0U; candidate < output_count; ++candidate) {
                if (outputs[candidate].type == mapping->type && strcmp(outputs[candidate].name, mapping->variable) == 0) {
                    mapping->value_reference = outputs[candidate].value_reference;
                    mapping->fmu_index = (uint16_t)candidate;
                    found = true;
                    break;
                }
            }
        }
        if (!found) return -1;
    }
    return 0;
}

int fmu_model_initialize_cosimulation(FmuModel *model, double start_time_s, double stop_time_s) {
    if (!model || !model->fmu || !model->dll_created) return -1;

    jm_status_enu_t jstatus = fmi2_import_instantiate(model->fmu, "realtime_instance", fmi2_cosimulation, NULL, fmi2_false);
    if (jstatus == jm_status_error) {
        fprintf(stderr, "fmi2_import_instantiate() failed.\n");
        return -1;
    }
    model->instantiated = true;

    fmi2_status_t status = fmi2_import_set_debug_logging(model->fmu, fmi2_false, 0, NULL);
    if (!status_is_ok(status)) fprintf(stderr, "Warning: failed to disable FMU debug logging.\n");

    status = fmi2_import_setup_experiment(model->fmu, fmi2_false, 0.0, start_time_s, fmi2_true, stop_time_s);
    if (!status_is_ok(status)) {
        fprintf(stderr, "fmi2_import_setup_experiment() failed.\n");
        return -1;
    }

    status = fmi2_import_enter_initialization_mode(model->fmu);
    if (!status_is_ok(status)) {
        fprintf(stderr, "fmi2_import_enter_initialization_mode() failed.\n");
        return -1;
    }

    status = fmi2_import_exit_initialization_mode(model->fmu);
    if (!status_is_ok(status)) {
        fprintf(stderr, "fmi2_import_exit_initialization_mode() failed.\n");
        return -1;
    }

    model->initialized = true;
    return 0;
}

int fmu_model_resolve_input_initial_values(FmuModel *model, input_channel_descriptor_t *inputs, size_t count) {
    if (!model || !model->fmu || !model->initialized || (count > 0U && !inputs)) return -1;

    for (size_t index = 0U; index < count; ++index) {
        const fmi2_value_reference_t value_reference = (fmi2_value_reference_t)inputs[index].value_reference;
        fmi2_status_t status;
        switch (inputs[index].type) {
            case NUMERIC_REAL: {
                fmi2_real_t value = 0.0;
                status = fmi2_import_get_real(model->fmu, &value_reference, 1U, &value);
                inputs[index].initial_value.real_value = (double)value;
                break;
            }
            case NUMERIC_INTEGER:
            case NUMERIC_ENUMERATION: {
                fmi2_integer_t value = 0;
                status = fmi2_import_get_integer(model->fmu, &value_reference, 1U, &value);
                inputs[index].initial_value.discrete_value = (int64_t)value;
                break;
            }
            case NUMERIC_BOOLEAN: {
                fmi2_boolean_t value = fmi2_false;
                status = fmi2_import_get_boolean(model->fmu, &value_reference, 1U, &value);
                inputs[index].initial_value.boolean_value = value == fmi2_true ? 1U : value == fmi2_false ? 0U : 2U;
                break;
            }
            default:
                return -1;
        }
        if (!status_is_ok(status)) return -1;
        inputs[index].initial_value_valid = true;
    }
    return 0;
}

int fmu_model_set_inputs(FmuModel *model, const input_channel_descriptor_t *inputs, const input_value_t *values, size_t count) {
    if (!model || !model->fmu || !model->initialized || (count > 0U && (!inputs || !values))) return -1;

    for (size_t index = 0U; index < count; ++index) {
        const fmi2_value_reference_t value_reference = (fmi2_value_reference_t)inputs[index].value_reference;
        fmi2_status_t status;
        switch (inputs[index].type) {
            case NUMERIC_REAL: {
                const fmi2_real_t value = (fmi2_real_t)values[index].real_value;
                status = fmi2_import_set_real(model->fmu, &value_reference, 1U, &value);
                break;
            }
            case NUMERIC_INTEGER:
            case NUMERIC_ENUMERATION: {
                const fmi2_integer_t value = (fmi2_integer_t)values[index].discrete_value;
                status = fmi2_import_set_integer(model->fmu, &value_reference, 1U, &value);
                break;
            }
            case NUMERIC_BOOLEAN: {
                const fmi2_boolean_t value = values[index].boolean_value == 0U ? fmi2_false : fmi2_true;
                status = fmi2_import_set_boolean(model->fmu, &value_reference, 1U, &value);
                break;
            }
            default:
                return -1;
        }
        if (!status_is_ok(status)) return -1;
    }
    return 0;
}

int fmu_model_do_step(FmuModel *model, double current_time_s, double step_size_s) {
    fmi2_status_t status = fmi2_import_do_step(model->fmu, current_time_s, step_size_s, fmi2_true);
    if (status == fmi2_status_pending) {
        fprintf(stderr, "Asynchronous fmi2DoStep (pending) is not supported by this real-time runner.\n");
        return -1;
    }
    if (!status_is_ok(status)) {
        fprintf(stderr, "fmi2DoStep failed at t=%.9f with status %s.\n", current_time_s, fmi2_status_to_string(status));
        return -1;
    }
    return 0;
}

int fmu_model_read_outputs(FmuModel *model, const OutputVariable *outputs, size_t count, log_value_t *values) {
    for (size_t i = 0; i < count; ++i) {
        fmi2_value_reference_t vr = (fmi2_value_reference_t)outputs[i].value_reference;
        fmi2_status_t status;

        switch (outputs[i].type) {
            case NUMERIC_REAL: {
                fmi2_real_t value = 0.0;
                status = fmi2_import_get_real(model->fmu, &vr, 1, &value);
                values[i].real_value = (double)value;
                break;
            }
            case NUMERIC_INTEGER:
            case NUMERIC_ENUMERATION: {
                fmi2_integer_t value = 0;
                status = fmi2_import_get_integer(model->fmu, &vr, 1, &value);
                values[i].discrete_value = (int64_t)value;
                break;
            }
            case NUMERIC_BOOLEAN: {
                fmi2_boolean_t value = fmi2_false;
                status = fmi2_import_get_boolean(model->fmu, &vr, 1, &value);
                values[i].boolean_value = value == fmi2_false ? 0U : value == fmi2_true ? 1U : 2U;
                break;
            }
            default:
                return -1;
        }

        if (!status_is_ok(status)) {
            fprintf(stderr, "Failed to read output '%s'.\n", outputs[i].name);
            return -1;
        }
    }
    return 0;
}

void fmu_model_terminate(FmuModel *model) {
    if (!model || !model->fmu || !model->instantiated) return;
    if (model->initialized) (void)fmi2_import_terminate(model->fmu);
    fmi2_import_free_instance(model->fmu);
    model->instantiated = false;
    model->initialized = false;
}

const char *fmu_model_name(const FmuModel *model) {
    return model->model_name;
}
