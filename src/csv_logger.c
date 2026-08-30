#include "csv_logger.h"
#include <stdio.h>
#include <time.h>

static void sleep_briefly(void) {
    struct timespec ts = {.tv_sec = 0, .tv_nsec = 1000000L};
    nanosleep(&ts, NULL);
}

static void *csv_logger_thread(void *arg) {
    CsvLoggerContext *context = arg;
    FILE *file = fopen(context->config->csv_path, "w");
    if (!file) {
        perror("fopen CSV");
        context->result = -1;
        return NULL;
    }

    fprintf(file, "sequence,simulation_time_s,wall_time_s");
    for (size_t i = 0; i < context->config->output_count; ++i) fprintf(file, ",\"%s\"", context->config->outputs[i].name);
    fputc('\n', file);

    SimulationSample sample;
    size_t pending_flush = 0;

    for (;;) {
        if (sample_queue_pop(context->queue, &sample)) {
            fprintf(file, "%llu,%.9f,%.9f", (unsigned long long)sample.sequence, sample.sim_time_s, sample.wall_time_s);
            for (size_t i = 0; i < sample.value_count; ++i) fprintf(file, ",%.17g", sample.values[i]);
            fputc('\n', file);
            if (++pending_flush >= 100) {
                fflush(file);
                pending_flush = 0;
            }
            continue;
        }

        if (atomic_load_explicit(context->producer_done, memory_order_acquire)) break;
        sleep_briefly();
    }

    fflush(file);
    fclose(file);
    context->result = 0;
    return NULL;
}

int csv_logger_start(CsvLoggerContext *context, SampleQueue *queue, const AppConfig *config, _Atomic bool *producer_done) {
    context->queue = queue;
    context->config = config;
    context->producer_done = producer_done;
    context->result = -1;
    return pthread_create(&context->thread, NULL, csv_logger_thread, context);
}

int csv_logger_join(CsvLoggerContext *context) {
    if (pthread_join(context->thread, NULL) != 0) return -1;
    return context->result;
}
