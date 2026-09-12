#ifndef SAMPLE_QUEUE_H
#define SAMPLE_QUEUE_H

/* Contrato público do módulo sample queue. */

#include <stdbool.h>
#include <stdatomic.h>
#include <stdint.h>
#include "common.h"

typedef struct {
    _Atomic size_t head;
    _Atomic size_t tail;
    _Atomic uint64_t dropped;
    SimulationSample slots[SAMPLE_QUEUE_CAPACITY];
} SampleQueue;

void sample_queue_init(SampleQueue *queue);
bool sample_queue_push(SampleQueue *queue, const SimulationSample *sample);
bool sample_queue_pop(SampleQueue *queue, SimulationSample *sample);
uint64_t sample_queue_dropped(const SampleQueue *queue);

#endif
