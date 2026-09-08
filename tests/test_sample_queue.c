#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sample_queue.h"

static void Require(bool condition, const char *message) {
    if (!condition) {
        fprintf(stderr, "%s\n", message);
        exit(EXIT_FAILURE);
    }
}

static SimulationSample MakeSample(uint64_t sequence) {
    SimulationSample sample = {0};

    sample.sequence = sequence;
    sample.sim_time_s = (double)sequence * 0.1;
    sample.wall_time_s = (double)sequence * 0.2;
    sample.value_count = 2U;
    sample.values[0] = (double)sequence;
    sample.values[1] = -(double)sequence;
    return sample;
}

static void RequireSameSample(const SimulationSample *expected, const SimulationSample *actual) {
    Require(actual->sequence == expected->sequence, "unexpected sample sequence");
    Require(actual->sim_time_s == expected->sim_time_s, "unexpected simulation time");
    Require(actual->wall_time_s == expected->wall_time_s, "unexpected wall time");
    Require(actual->value_count == expected->value_count, "unexpected value count");
    Require(memcmp(actual->values, expected->values, sizeof(expected->values)) == 0, "unexpected sample values");
}

static void TestOrder(void) {
    SampleQueue queue;
    SimulationSample actual;
    const SimulationSample first = MakeSample(UINT64_C(11));
    const SimulationSample second = MakeSample(UINT64_C(12));
    const SimulationSample third = MakeSample(UINT64_C(13));

    sample_queue_init(&queue);
    Require(!sample_queue_pop(&queue, &actual), "empty queue accepted a pop");
    Require(sample_queue_push(&queue, &first), "first sample was rejected");
    Require(sample_queue_push(&queue, &second), "second sample was rejected");
    Require(sample_queue_push(&queue, &third), "third sample was rejected");

    Require(sample_queue_pop(&queue, &actual), "first sample was not available");
    RequireSameSample(&first, &actual);
    Require(sample_queue_pop(&queue, &actual), "second sample was not available");
    RequireSameSample(&second, &actual);
    Require(sample_queue_pop(&queue, &actual), "third sample was not available");
    RequireSameSample(&third, &actual);
    Require(!sample_queue_pop(&queue, &actual), "queue remained nonempty after ordered pops");
    Require(sample_queue_dropped(&queue) == UINT64_C(0), "ordered queue reported dropped samples");
}

static void TestWrap(void) {
    SampleQueue queue;
    SimulationSample actual;
    const size_t half_capacity = SAMPLE_QUEUE_CAPACITY / 2U;

    sample_queue_init(&queue);
    for (size_t index = 0; index < SAMPLE_QUEUE_CAPACITY - 1U; ++index) {
        const SimulationSample expected = MakeSample((uint64_t)index);
        Require(sample_queue_push(&queue, &expected), "initial sample was rejected before wrap");
    }

    for (size_t index = 0; index < half_capacity; ++index) {
        const SimulationSample expected = MakeSample((uint64_t)index);
        Require(sample_queue_pop(&queue, &actual), "sample was unavailable before wrap");
        RequireSameSample(&expected, &actual);
    }

    for (size_t index = 0; index < half_capacity; ++index) {
        const SimulationSample expected = MakeSample((uint64_t)(SAMPLE_QUEUE_CAPACITY - 1U + index));
        Require(sample_queue_push(&queue, &expected), "sample was rejected while wrapping");
    }

    for (size_t index = half_capacity; index < SAMPLE_QUEUE_CAPACITY - 1U + half_capacity; ++index) {
        const SimulationSample expected = MakeSample((uint64_t)index);
        Require(sample_queue_pop(&queue, &actual), "wrapped sample was unavailable");
        RequireSameSample(&expected, &actual);
    }

    Require(!sample_queue_pop(&queue, &actual), "queue remained nonempty after wrap test");
    Require(sample_queue_dropped(&queue) == UINT64_C(0), "wrap test reported dropped samples");
}

static void TestSaturation(void) {
    SampleQueue queue;
    SimulationSample actual;

    sample_queue_init(&queue);
    for (size_t index = 0; index < SAMPLE_QUEUE_CAPACITY - 1U; ++index) {
        const SimulationSample expected = MakeSample((uint64_t)index);
        Require(sample_queue_push(&queue, &expected), "sample was rejected before saturation");
    }

    const SimulationSample rejected = MakeSample((uint64_t)(SAMPLE_QUEUE_CAPACITY - 1U));
    Require(!sample_queue_push(&queue, &rejected), "saturated queue accepted a sample");
    Require(sample_queue_dropped(&queue) == UINT64_C(1), "saturated queue did not count the rejected sample");

    const SimulationSample first = MakeSample(UINT64_C(0));
    Require(sample_queue_pop(&queue, &actual), "saturated queue did not retain its first sample");
    RequireSameSample(&first, &actual);
    Require(sample_queue_dropped(&queue) == UINT64_C(1), "pop changed the dropped-sample count");
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <order|wrap|saturation>\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "order") == 0) {
        TestOrder();
    } else if (strcmp(argv[1], "wrap") == 0) {
        TestWrap();
    } else if (strcmp(argv[1], "saturation") == 0) {
        TestSaturation();
    } else {
        fprintf(stderr, "unknown test: %s\n", argv[1]);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
