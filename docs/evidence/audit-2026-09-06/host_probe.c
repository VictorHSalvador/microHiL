#include "app_config.h"
#include "csv_logger.h"
#include "sample_queue.h"
#include <assert.h>
#include <stdio.h>

/* Audit probe, not a product acceptance suite. */
static SampleQueue queue;

int main(void) {
    SimulationSample input = {0}, output = {0};
    sample_queue_init(&queue);
    assert(!sample_queue_pop(&queue, &output));
    for (size_t round = 0; round < 3; ++round) {
        for (size_t i = 0; i < SAMPLE_QUEUE_CAPACITY - 1; ++i) {
            input.sequence = round * SAMPLE_QUEUE_CAPACITY + i;
            assert(sample_queue_push(&queue, &input));
        }
        assert(!sample_queue_push(&queue, &input));
        for (size_t i = 0; i < SAMPLE_QUEUE_CAPACITY - 1; ++i) {
            assert(sample_queue_pop(&queue, &output));
            assert(output.sequence == round * SAMPLE_QUEUE_CAPACITY + i);
        }
        assert(!sample_queue_pop(&queue, &output));
    }
    assert(sample_queue_dropped(&queue) == 3);
    printf("QUEUE: PASS (empty, FIFO, 4095 usable slots, wrap, 3 drops)\n");
    printf("sizeof(SimulationSample)=%zu sizeof(SampleQueue)=%zu\n",
           sizeof(SimulationSample), sizeof(SampleQueue));

    AppConfig config;
    app_config_set_defaults(&config);
    snprintf(config.csv_path, sizeof(config.csv_path), "/dev/full");
    config.output_count = 1;
    snprintf(config.outputs[0].name, sizeof(config.outputs[0].name), "probe");
    sample_queue_init(&queue);
    input.value_count = 1;
    input.values[0] = 42.0;
    assert(sample_queue_push(&queue, &input));
    _Atomic bool done = true;
    CsvLoggerContext logger;
    assert(csv_logger_start(&logger, &queue, &config, &done) == 0);
    int result = csv_logger_join(&logger);
    printf("LOGGER /dev/full: expected nonzero error; observed %d => %s\n",
           result, result == 0 ? "FAIL (write failure hidden)" : "PASS");
    return result == 0 ? 1 : 0;
}
