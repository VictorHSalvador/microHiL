#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>
#include <stdint.h>

#define MAX_OUTPUTS 64
#define OUTPUT_NAME_LEN 192
#define PATH_LEN 512
#define SAMPLE_QUEUE_CAPACITY 4096

typedef enum {
    NUMERIC_REAL = 0,
    NUMERIC_INTEGER,
    NUMERIC_BOOLEAN,
    NUMERIC_ENUMERATION
} NumericType;

typedef struct {
    char name[OUTPUT_NAME_LEN];
    uint32_t value_reference;
    NumericType type;
} OutputVariable;

typedef struct {
    uint64_t sequence;
    double sim_time_s;
    double wall_time_s;
    size_t value_count;
    double values[MAX_OUTPUTS];
} SimulationSample;

#endif
