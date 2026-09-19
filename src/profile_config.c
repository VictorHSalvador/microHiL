/* Implementação do módulo profile config. */
#define _GNU_SOURCE
#include "profile_config.h"

#include <errno.h>
#include <limits.h>
#include <locale.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <yaml.h>

static yaml_node_t *MappingValue(yaml_document_t *document, yaml_node_t *mapping, const char *key) {
    if (!document || !mapping || mapping->type != YAML_MAPPING_NODE) return NULL;
    for (yaml_node_pair_t *pair = mapping->data.mapping.pairs.start; pair < mapping->data.mapping.pairs.top; ++pair) {
        yaml_node_t *candidate = yaml_document_get_node(document, pair->key);
        if (candidate && candidate->type == YAML_SCALAR_NODE && strcmp((const char *)candidate->data.scalar.value, key) == 0) {
            return yaml_document_get_node(document, pair->value);
        }
    }
    return NULL;
}

static const char *Scalar(yaml_node_t *node) {
    return node && node->type == YAML_SCALAR_NODE ? (const char *)node->data.scalar.value : NULL;
}

static bool ParseUnsigned(yaml_node_t *node, unsigned int *value) {
    const char *text = Scalar(node);
    char *end = NULL;
    if (!text || !value) return false;
    errno = 0;
    unsigned long parsed = strtoul(text, &end, 10);
    if (errno || !end || *end != '\0' || parsed > UINT_MAX) return false;
    *value = (unsigned int)parsed;
    return true;
}

static bool ParseUint8(yaml_node_t *node, uint8_t *value) {
    unsigned int parsed = 0U;
    if (!value || !ParseUnsigned(node, &parsed) || parsed > UINT8_MAX) return false;
    *value = (uint8_t)parsed;
    return true;
}

static bool ParseUint16(yaml_node_t *node, uint16_t *value) {
    unsigned int parsed = 0U;
    if (!value || !ParseUnsigned(node, &parsed) || parsed > UINT16_MAX) return false;
    *value = (uint16_t)parsed;
    return true;
}

static bool ParseUint32(yaml_node_t *node, uint32_t *value) {
    const char *text = Scalar(node);
    char *end = NULL;
    if (!text || !value) return false;
    errno = 0;
    const unsigned long parsed = strtoul(text, &end, 10);
    if (errno || !end || *end != '\0' || parsed > UINT32_MAX) return false;
    *value = (uint32_t)parsed;
    return true;
}

static bool ParseDouble(yaml_node_t *node, double *value) {
    const char *text = Scalar(node);
    char *end = NULL;
    if (!text || !value) return false;
    locale_t c_locale = newlocale(LC_NUMERIC_MASK, "C", (locale_t)0);
    if (!c_locale) return false;
    errno = 0;
    *value = strtod_l(text, &end, c_locale);
    freelocale(c_locale);
    return !errno && end && *end == '\0' && isfinite(*value);
}

static bool ParseType(const char *text, NumericType *type) {
    if (!text || !type) return false;
    if (strcmp(text, "real") == 0) *type = NUMERIC_REAL;
    else if (strcmp(text, "integer") == 0) *type = NUMERIC_INTEGER;
    else if (strcmp(text, "boolean") == 0) *type = NUMERIC_BOOLEAN;
    else if (strcmp(text, "enumeration") == 0) *type = NUMERIC_ENUMERATION;
    else return false;
    return true;
}

static bool IsEsp32Channel(const char *channel) {
    static const char *const names[] = {
        "GPIO4_DI", "GPIO13_DI", "GPIO14_DI", "GPIO27_DI", "GPIO32_AI", "GPIO33_AI", "GPIO34_AI", "GPIO35_AI", "GPIO36_AI", "GPIO39_AI",
        "GPIO16_DO", "GPIO17_DO", "GPIO18_PWM", "GPIO19_PWM", "GPIO21_DO", "GPIO22_DO", "GPIO23_DO", "GPIO25_AO", "GPIO26_AO"
    };
    for (size_t index = 0U; index < sizeof(names) / sizeof(names[0]); ++index) if (strcmp(channel, names[index]) == 0) return true;
    return false;
}

static bool IsEsp32ChannelTypeCompatible(const char *channel, NumericType type) {
    const bool analog_channel = strstr(channel, "_AI") != NULL || strstr(channel, "_AO") != NULL || strstr(channel, "_PWM") != NULL;
    const bool digital_channel = strstr(channel, "_DI") != NULL || strstr(channel, "_DO") != NULL;
    return (analog_channel && type == NUMERIC_REAL) || (digital_channel && type == NUMERIC_BOOLEAN);
}

static bool HasOnlyKeys(yaml_document_t *document, yaml_node_t *mapping, const char *const *keys, size_t key_count) {
    if (!document || !mapping || mapping->type != YAML_MAPPING_NODE) return false;
    size_t found[8] = {0};
    if (key_count > sizeof(found) / sizeof(found[0])) return false;
    for (yaml_node_pair_t *pair = mapping->data.mapping.pairs.start; pair < mapping->data.mapping.pairs.top; ++pair) {
        const char *key = Scalar(yaml_document_get_node(document, pair->key));
        bool known = false;
        for (size_t index = 0U; index < key_count; ++index) {
            if (key && strcmp(key, keys[index]) == 0) {
                ++found[index];
                known = true;
                break;
            }
        }
        if (!known) return false;
    }
    for (size_t index = 0U; index < key_count; ++index) if (found[index] != 1U) return false;
    return true;
}

static bool ParseDaqcConfiguration(yaml_document_t *document, yaml_node_t *root, profile_daqc_configuration_t *configuration) {
    static const char *const adc_keys[] = {"resolution_bits", "attenuation"};
    static const char *const acquisition_keys[] = {"frequency_hz"};
    static const char *const attenuation_channels[] = {"GPIO32_AI", "GPIO33_AI", "GPIO34_AI", "GPIO35_AI", "GPIO36_AI", "GPIO39_AI"};
    static const char *const pwm_channels[] = {"GPIO18_PWM", "GPIO19_PWM"};
    static const char *const pwm_keys[] = {"frequency_hz", "resolution_bits"};
    yaml_node_t *adc = MappingValue(document, root, "adc");
    yaml_node_t *pwm = MappingValue(document, root, "pwm");
    yaml_node_t *acquisition = MappingValue(document, root, "acquisition");
    yaml_node_t *attenuation = MappingValue(document, adc, "attenuation");
    if (!configuration || !HasOnlyKeys(document, adc, adc_keys, sizeof(adc_keys) / sizeof(adc_keys[0])) ||
        !HasOnlyKeys(document, attenuation, attenuation_channels, sizeof(attenuation_channels) / sizeof(attenuation_channels[0])) ||
        !HasOnlyKeys(document, pwm, pwm_channels, sizeof(pwm_channels) / sizeof(pwm_channels[0])) ||
        !ParseUint8(MappingValue(document, adc, "resolution_bits"), &configuration->adc_resolution_bits) ||
        configuration->adc_resolution_bits < 9U || configuration->adc_resolution_bits > 12U) {
        return false;
    }
    for (size_t index = 0U; index < PROFILE_CONFIG_ADC_CHANNELS; ++index) {
        if (!ParseUint8(MappingValue(document, attenuation, attenuation_channels[index]), &configuration->adc_attenuation[index]) ||
            configuration->adc_attenuation[index] > 3U) return false;
    }
    for (size_t index = 0U; index < PROFILE_CONFIG_PWM_CHANNELS; ++index) {
        yaml_node_t *channel = MappingValue(document, pwm, pwm_channels[index]);
        if (!HasOnlyKeys(document, channel, pwm_keys, sizeof(pwm_keys) / sizeof(pwm_keys[0])) ||
            !ParseUint32(MappingValue(document, channel, "frequency_hz"), &configuration->pwm_frequency_hz[index]) ||
            !ParseUint8(MappingValue(document, channel, "resolution_bits"), &configuration->pwm_resolution_bits[index]) ||
            configuration->pwm_frequency_hz[index] == 0U || configuration->pwm_resolution_bits[index] == 0U ||
            configuration->pwm_resolution_bits[index] > 20U) return false;
    }
    configuration->acquisition_frequency_hz = PROFILE_CONFIG_DEFAULT_ACQUISITION_FREQUENCY_HZ;
    if (!acquisition) return true;
    if (!HasOnlyKeys(document, acquisition, acquisition_keys, sizeof(acquisition_keys) / sizeof(acquisition_keys[0])) ||
        !ParseUint16(MappingValue(document, acquisition, "frequency_hz"), &configuration->acquisition_frequency_hz) ||
        configuration->acquisition_frequency_hz == 0U || configuration->acquisition_frequency_hz > PROFILE_CONFIG_MAX_ACQUISITION_FREQUENCY_HZ) return false;
    return true;
}

typedef struct {
    const char *name;
    uint8_t gpio;
    daq_channel_function_t function;
    daq_wire_type_t wire_type;
    uint16_t offset;
    uint16_t width;
} acquisition_channel_t;

static bool BuildAcquisitionField(const profile_mapping_t *mapping, daq_field_t *field) {
    static const acquisition_channel_t channels[] = {
        {"GPIO4_DI", 4U, DAQ_CHANNEL_DI, DAQ_WIRE_BOOLEAN, 0U, 1U}, {"GPIO13_DI", 13U, DAQ_CHANNEL_DI, DAQ_WIRE_BOOLEAN, 1U, 1U},
        {"GPIO14_DI", 14U, DAQ_CHANNEL_DI, DAQ_WIRE_BOOLEAN, 2U, 1U}, {"GPIO27_DI", 27U, DAQ_CHANNEL_DI, DAQ_WIRE_BOOLEAN, 3U, 1U},
        {"GPIO32_AI", 32U, DAQ_CHANNEL_AI, DAQ_WIRE_FLOAT32, 4U, 4U}, {"GPIO33_AI", 33U, DAQ_CHANNEL_AI, DAQ_WIRE_FLOAT32, 8U, 4U},
        {"GPIO34_AI", 34U, DAQ_CHANNEL_AI, DAQ_WIRE_FLOAT32, 12U, 4U}, {"GPIO35_AI", 35U, DAQ_CHANNEL_AI, DAQ_WIRE_FLOAT32, 16U, 4U},
        {"GPIO36_AI", 36U, DAQ_CHANNEL_AI, DAQ_WIRE_FLOAT32, 20U, 4U}, {"GPIO39_AI", 39U, DAQ_CHANNEL_AI, DAQ_WIRE_FLOAT32, 24U, 4U}
    };
    if (!mapping || !field) return false;
    for (size_t index = 0U; index < sizeof(channels) / sizeof(channels[0]); ++index) {
        if (strcmp(mapping->channel, channels[index].name) != 0) continue;
        *field = (daq_field_t){
            .gpio = channels[index].gpio,
            .function = channels[index].function,
            .wire_type = channels[index].wire_type,
            .fmu_type = mapping->type,
            .offset = channels[index].offset,
            .width = channels[index].width,
            .scale = mapping->scale,
            .offset_value = mapping->offset,
            .value_reference = mapping->value_reference,
            .fmu_index = mapping->fmu_index
        };
        return true;
    }
    return false;
}

static bool BuildActuationField(const profile_mapping_t *mapping, daq_field_t *field) {
    static const acquisition_channel_t channels[] = {
        {"GPIO16_DO", 16U, DAQ_CHANNEL_DO, DAQ_WIRE_BOOLEAN, 0U, 1U}, {"GPIO17_DO", 17U, DAQ_CHANNEL_DO, DAQ_WIRE_BOOLEAN, 1U, 1U},
        {"GPIO18_PWM", 18U, DAQ_CHANNEL_PWM, DAQ_WIRE_FLOAT32, 2U, 4U}, {"GPIO19_PWM", 19U, DAQ_CHANNEL_PWM, DAQ_WIRE_FLOAT32, 6U, 4U},
        {"GPIO21_DO", 21U, DAQ_CHANNEL_DO, DAQ_WIRE_BOOLEAN, 10U, 1U}, {"GPIO22_DO", 22U, DAQ_CHANNEL_DO, DAQ_WIRE_BOOLEAN, 11U, 1U},
        {"GPIO23_DO", 23U, DAQ_CHANNEL_DO, DAQ_WIRE_BOOLEAN, 12U, 1U}, {"GPIO25_AO", 25U, DAQ_CHANNEL_AO, DAQ_WIRE_FLOAT32, 13U, 4U},
        {"GPIO26_AO", 26U, DAQ_CHANNEL_AO, DAQ_WIRE_FLOAT32, 17U, 4U}
    };
    if (!mapping || !field) return false;
    for (size_t index = 0U; index < sizeof(channels) / sizeof(channels[0]); ++index) {
        if (strcmp(mapping->channel, channels[index].name) != 0) continue;
        *field = (daq_field_t){
            .gpio = channels[index].gpio,
            .function = channels[index].function,
            .wire_type = channels[index].wire_type,
            .fmu_type = mapping->type,
            .offset = channels[index].offset,
            .width = channels[index].width,
            .scale = mapping->scale,
            .offset_value = mapping->offset,
            .value_reference = mapping->value_reference,
            .fmu_index = mapping->fmu_index
        };
        return true;
    }
    return false;
}

static void SortFieldsByOffset(daq_field_t *fields, size_t field_count) {
    for (size_t index = 1U; index < field_count; ++index) {
        daq_field_t current = fields[index];
        size_t target = index;
        while (target > 0U && fields[target - 1U].offset > current.offset) {
            fields[target] = fields[target - 1U];
            --target;
        }
        fields[target] = current;
    }
}

profile_config_status_t ProfileConfigLoadYaml(const char *path, profile_config_t *config) {
    if (!path || !*path || !config) return PROFILE_CONFIG_INVALID_ARGUMENT;
    FILE *file = fopen(path, "rb");
    if (!file) return PROFILE_CONFIG_IO;
    yaml_parser_t parser;
    yaml_document_t document;
    if (!yaml_parser_initialize(&parser)) {
        fclose(file);
        return PROFILE_CONFIG_IO;
    }
    yaml_parser_set_input_file(&parser, file);
    const int loaded = yaml_parser_load(&parser, &document);
    yaml_parser_delete(&parser);
    fclose(file);
    if (!loaded) return PROFILE_CONFIG_SYNTAX;

    *config = (profile_config_t){0};
    yaml_node_t *root = yaml_document_get_root_node(&document);
    yaml_node_t *execution = MappingValue(&document, root, "execution");
    yaml_node_t *profile = MappingValue(&document, root, "profile");
    yaml_node_t *mappings = MappingValue(&document, root, "mappings");
    if (!ParseUnsigned(MappingValue(&document, root, "version"), &config->version) || !ParseUnsigned(MappingValue(&document, profile, "id"), &config->profile_id) ||
        !ParseDouble(MappingValue(&document, execution, "step_size_s"), &config->step_size_s) || !ParseDouble(MappingValue(&document, execution, "stop_time_s"), &config->stop_time_s) ||
        !mappings || mappings->type != YAML_SEQUENCE_NODE || !ParseDaqcConfiguration(&document, root, &config->daqc_configuration)) {
        yaml_document_delete(&document);
        return PROFILE_CONFIG_SCHEMA;
    }
    if (config->version != PROFILE_CONFIG_VERSION || config->profile_id != 1U || config->step_size_s <= 0.0 || config->stop_time_s <= 0.0) {
        yaml_document_delete(&document);
        return PROFILE_CONFIG_UNSUPPORTED;
    }
    for (yaml_node_item_t *item = mappings->data.sequence.items.start; item < mappings->data.sequence.items.top; ++item) {
        if (config->mapping_count == PROFILE_CONFIG_MAX_MAPPINGS) {
            yaml_document_delete(&document);
            return PROFILE_CONFIG_SCHEMA;
        }
        yaml_node_t *mapping = yaml_document_get_node(&document, *item);
        profile_mapping_t *entry = &config->mappings[config->mapping_count];
        const char *channel = Scalar(MappingValue(&document, mapping, "channel"));
        const char *variable = Scalar(MappingValue(&document, mapping, "variable"));
        if (!channel || !variable || !IsEsp32Channel(channel) || strlen(channel) >= sizeof(entry->channel) || strlen(variable) >= sizeof(entry->variable) ||
            !ParseType(Scalar(MappingValue(&document, mapping, "type")), &entry->type) || !ParseDouble(MappingValue(&document, mapping, "scale"), &entry->scale) ||
            !ParseDouble(MappingValue(&document, mapping, "offset"), &entry->offset) || entry->scale == 0.0 || !IsEsp32ChannelTypeCompatible(channel, entry->type)) {
            yaml_document_delete(&document);
            return PROFILE_CONFIG_SCHEMA;
        }
        snprintf(entry->channel, sizeof(entry->channel), "%s", channel);
        snprintf(entry->variable, sizeof(entry->variable), "%s", variable);
        ++config->mapping_count;
    }
    yaml_document_delete(&document);
    return PROFILE_CONFIG_OK;
}

const char *ProfileConfigStatusString(profile_config_status_t status) {
    switch (status) {
        case PROFILE_CONFIG_OK: return "ok";
        case PROFILE_CONFIG_INVALID_ARGUMENT: return "invalid argument";
        case PROFILE_CONFIG_IO: return "could not read YAML configuration";
        case PROFILE_CONFIG_SYNTAX: return "invalid YAML syntax";
        case PROFILE_CONFIG_SCHEMA: return "invalid YAML configuration schema";
        case PROFILE_CONFIG_UNSUPPORTED: return "unsupported configuration version or profile";
        default: return "unknown configuration status";
    }
}

profile_config_status_t ProfileConfigBuildAcquisitionSchema(const profile_config_t *config, daq_schema_t *schema) {
    if (!config || !schema || config->profile_id != 1U) return PROFILE_CONFIG_INVALID_ARGUMENT;
    daq_field_t fields[10];
    size_t count = 0U;
    for (size_t index = 0U; index < config->mapping_count; ++index) {
        const profile_mapping_t *mapping = &config->mappings[index];
        if (!mapping->is_input) continue;
        daq_field_t field;
        if (count == sizeof(fields) / sizeof(fields[0]) || !BuildAcquisitionField(mapping, &field)) return PROFILE_CONFIG_SCHEMA;
        fields[count++] = field;
    }
    SortFieldsByOffset(fields, count);
    return DaqSchemaBuildFixedPayload(schema, fields, count, 28U) == DAQ_SCHEMA_OK ? PROFILE_CONFIG_OK : PROFILE_CONFIG_SCHEMA;
}

profile_config_status_t ProfileConfigBuildActuationSchema(const profile_config_t *config, daq_schema_t *schema) {
    if (!config || !schema || config->profile_id != 1U) return PROFILE_CONFIG_INVALID_ARGUMENT;
    daq_field_t fields[9];
    size_t count = 0U;
    for (size_t index = 0U; index < config->mapping_count; ++index) {
        const profile_mapping_t *mapping = &config->mappings[index];
        if (mapping->is_input) continue;
        daq_field_t field;
        if (count == sizeof(fields) / sizeof(fields[0]) || !BuildActuationField(mapping, &field)) return PROFILE_CONFIG_SCHEMA;
        fields[count++] = field;
    }
    SortFieldsByOffset(fields, count);
    return DaqSchemaBuildFixedPayload(schema, fields, count, 21U) == DAQ_SCHEMA_OK ? PROFILE_CONFIG_OK : PROFILE_CONFIG_SCHEMA;
}
