#ifndef PLOTTER_H
#define PLOTTER_H

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
} PlotterContext;

int plotter_start(PlotterContext *context, SampleQueue *queue, const AppConfig *config, _Atomic bool *producer_done);
int plotter_join(PlotterContext *context);

#endif
