#ifndef CSV_LOGGER_H
#define CSV_LOGGER_H

#include <pthread.h>
#include <stdatomic.h>
#include "app_config.h"
#include "sample_queue.h"

typedef struct {
    pthread_t thread;
    SampleQueue *queue;
    const AppConfig *config;
    _Atomic bool *producer_done;
    int result;
} CsvLoggerContext;

int csv_logger_start(CsvLoggerContext *context, SampleQueue *queue, const AppConfig *config, _Atomic bool *producer_done);
int csv_logger_join(CsvLoggerContext *context);

#endif
