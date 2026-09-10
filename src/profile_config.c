#include "profile_config.h"

#include <errno.h>
#include <limits.h>
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

static bool ParseDouble(yaml_node_t *node, double *value) {
    const char *text = Scalar(node);
    char *end = NULL;
    if (!text || !value) return false;
    errno = 0;
    *value = strtod(text, &end);
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
        !mappings || mappings->type != YAML_SEQUENCE_NODE) {
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
            !ParseDouble(MappingValue(&document, mapping, "offset"), &entry->offset) || entry->scale == 0.0) {
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
