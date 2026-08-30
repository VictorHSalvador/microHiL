#ifndef RT_SIMULATION_H
#define RT_SIMULATION_H

#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include "app_config.h"
#include "fmu_model.h"
#include "sample_queue.h"

typedef struct {
    uint64_t completed_steps;
    uint64_t deadline_misses;
    double max_computation_s;
    double max_lateness_s;
    bool sched_fifo_active;
} SimulationStats;

typedef struct {
    pthread_t thread;
    FmuModel *model;
    const AppConfig *config;
    SampleQueue *log_queue;
    SampleQueue *plot_queue;
    _Atomic bool *stop_requested;
    _Atomic bool *producer_done;
    SimulationStats stats;
    int result;
} RtSimulationContext;

int rt_simulation_start(RtSimulationContext *context, FmuModel *model, const AppConfig *config,
                        SampleQueue *log_queue, SampleQueue *plot_queue,
                        _Atomic bool *stop_requested, _Atomic bool *producer_done);
int rt_simulation_join(RtSimulationContext *context);

#endif
